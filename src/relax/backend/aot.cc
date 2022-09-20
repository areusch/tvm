#include <utility>
#include <vector>

#include <tvm/runtime/object.h>
#include <tvm/relay/attrs/memory.h>
#include <tvm/relax/expr_functor.h>
#include <tvm/relax/op_attr_types.h>
#include <tvm/tir/analysis.h>
#include <tvm/tir/buffer.h>
#include <tvm/tir/builtin.h>
#include <tvm/tir/function.h>
#include <tvm/tir/transform.h>
#include <tvm/tir/var.h>

#include "../../relay/backend/utils.h"
#include "../../printer/text_printer.h"

namespace tvm {
namespace relax {
namespace backend {

using FunctionInfo = ::tvm::relay::backend::FunctionInfo;
using LoweredOutput = ::tvm::relay::backend::LoweredOutput;

namespace aot_op {
static const Op& call_tir_op = Op::Get("relax.call_tir");
static const Op& alloc_tensor_op = Op::Get("relax.builtin.alloc_tensor");
static const Op& call_tir_dyn_op = Op::Get("relax.vm.call_tir_dyn");

//static const Op& alloc_tensor_op = Op::Get("relax.aot.alloc_tensor");
//static const Op& call_tir_op = Op::Get("relax.aot.call_packed");
}  // namespcae aot_op

namespace {

// Helper function to get the function name of the registered packed function implementation of
// relax operator.
FCallPacked GetPackedFuncName(const Call& call) {
  auto op_map = Op::GetAttrMap<FCallPacked>("FCallPacked");
  if (call->op.as<OpNode>()) {
    Op op = Downcast<Op>(call->op);
    if (op_map.count(op)) {
      return op_map[op];
    }
  }
  return {};
}

}  // namespace


/*!
 * \brief Tracks a single value manifested in the Relax program.
 *
 * This class is ultimately responsible for determining how to create TIR buffers to form backing
 * memory for the Relax value (hereafter termed "implementing" the value). On initial creation, this
 * class simply serves as a central reference point to de-duplicated aliases and serve as a Relax-
 * level placeholder when tracing the dataflow in ScopeCollector.
 *
 * Before synthesizing the TIR main, Implement() must be called to create Buffers. Right now, Buffers are created
 * only when a tir.allocate node needs to be created (see is_output()). This should probably be factored out
 * so that the same Buffer-lowering logic is used for the I/O vars as well (is_output() is probably poorly-named/placed).
 */
class RealizedExprNode : public Object {
 public:
  Expr expr;
  String name_hint;
  bool use_unique_name;

  unsigned int NumVars() {
    ICHECK(is_implemented_) << "Cannot use NumVars() until Implement() is called";
    return vars_.size();
  }

  tir::Var Var(unsigned int i) {
    ICHECK(is_implemented_) << "Cannot use Vars() until Implement() is called";
    CHECK_LT(i, vars_.size()) << "Buffer out of range";
    return vars_[i];
  }

  tir::Buffer Buffer(unsigned int i) {
    ICHECK(is_implemented_) << "Cannot use Buffer() until Implement() is called";
    CHECK_LT(i, vars_.size()) << "Buffer out of range";
    return buffers_[i];
  }

  bool is_output() const {
    return is_implemented_ && buffers_.empty();
  }

  inline bool is_implemented() const {
    return is_implemented_;
  }

  void Implement_(NameSupply name_supply, bool decl_buffers, Expr expr) {
    if (const DynTensorTypeNode* dyn_tensor = expr->checked_type().as<DynTensorTypeNode>()) {
      std::string name = name_hint;
      if (name.empty()) {
        name = "buf";
      }
      if (decl_buffers) {
        String buffer_name(name_supply->FreshName(name));
        buffers_.push_back(tir::decl_buffer(Downcast<ShapeExpr>(expr->shape_)->values,
                                            dyn_tensor->dtype,
                                            name,
                                            "global"  /* storage_scope= */));
        vars_.push_back(tir::Var(buffer_name, PointerType(PrimType(dyn_tensor->dtype), String("global"))));
      } else {
        vars_.push_back(tir::Var(name, PointerType(PrimType(dyn_tensor->dtype), String("global"))));
      }
      is_implemented_ = true;
    } else {
      ICHECK(false) << "Don't know how to implement Relax Expr " << expr;
    }
  }

  void Implement(NameSupply name_supply, bool decl_buffers) {
    CHECK(!is_implemented_) << "Cannot implement twice";
    if (const TupleNode* tuple = expr.as<TupleNode>()) {
      for (auto field : tuple->fields) {
        Implement_(name_supply, decl_buffers, field);
      }
    } else {
      Implement_(name_supply, decl_buffers, expr);
    }
  }

 private:
  bool is_implemented_;
  Array<tir::Buffer> buffers_;
  Array<tir::Var> vars_;
 public:
  TVM_DECLARE_FINAL_OBJECT_INFO(RealizedExprNode, Object);
};

class RealizedExpr : public ObjectRef {
 public:
  RealizedExpr(Expr expr, String name_hint, bool use_unique_name=false) {
    auto n = make_object<RealizedExprNode>();
    n->expr = expr;
    n->name_hint = name_hint;
    n->use_unique_name = use_unique_name;
    data_ = std::move(n);
  }

  TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(RealizedExpr, ObjectRef, RealizedExprNode);
};

std::ostream& operator<<(std::ostream& os, const RealizedExpr& re) {
  os << "RealizedExpr(name_hint=" << re->name_hint << ", use_unique_name=" << re->use_unique_name << ", expr=" << re->expr << ")";
  return os;
}

/*! Tracks all RealizedExpr and implements manipulations on the lookup table. */
class ScopeProjectionNode : public Object {
 public:
  Map<ObjectRef, RealizedExpr> allocs;

  /*! \brief Create RealizedExpr on discovery of new Relax Expr with a unique value .*/
  RealizedExpr Realize(Expr expr, String name_hint = "") {
    auto re = RealizedExpr(expr, name_hint);
    allocs.Set(expr, re);
    return re;
  }

  /*! \brief Bind previously-created RealizedExpr to a new Relax variable (e.g. alias it). */
  void Bind(RealizedExpr re, Var binding) {
    CHECK(allocs.find(binding) == allocs.end());

    allocs.Set(binding, re);
    if (re->name_hint.empty() && !binding->name_hint().empty()) {
      re->name_hint = binding->name_hint();
    }
  }

  RealizedExpr Lookup(Expr e) const {
    auto it = allocs.find(e);
    CHECK(it != allocs.end()) << "Unable to find expr " << RelaxScriptPrinter(false, nullptr).Print(e).str();
    return (*it).second;
  }

  TVM_DECLARE_FINAL_OBJECT_INFO(ScopeProjectionNode, Object);
};

class ScopeProjection : public ObjectRef {
 public:
  TVM_DEFINE_MUTABLE_OBJECT_REF_METHODS(ScopeProjection, ObjectRef, ScopeProjectionNode);
};

std::ostream& operator<<(std::ostream& os, const ScopeProjection& proj) {
  os << "ScopeProjection(";
  bool is_zeroth = true;
  for (auto it : proj->allocs) {
    if (!is_zeroth) {
      os << ", ";
    } else {
      is_zeroth = false;
    }
    os << RelaxScriptPrinter(false, nullptr).Print(it.first).str() << "=" << it.second;
  }
  os << ")";
  return os;
}

/*! \brief An ExprFunctor that properly visits the whole program and returns a value from all Expr. */
template <typename R>
class DefaultExprFunctor : public ExprFunctor<R(const Expr& n)> {
 protected:

  virtual R DefaultResult() = 0;

  R VisitExpr_(const ConstantNode* op) override {
    this->VisitSpan(op->span);

    if (op->shape_) {
      this->VisitExpr(Downcast<Expr>(op->shape_.value()));
    }

    return DefaultResult();
  }

  R VisitExpr_(const GlobalVarNode* op) override {
    this->VisitSpan(op->span);
    return DefaultResult();
  }

  R VisitExpr_(const TupleNode* op) override {
    this->VisitSpan(op->span);
    for (Expr field : op->fields) {
      this->VisitExpr(field);
    }

    if (op->shape_) {
      this->VisitExpr(Downcast<Expr>(op->shape_.value()));
    }

    return DefaultResult();
  }

  // Visit the use-site of a defined Var
  R VisitExpr_(const VarNode* op) override {
    this->VisitSpan(op->span);
    return DefaultResult();
  }

// Visit the use-site of a defined DataflowVar
  R VisitExpr_(const DataflowVarNode* op) override {
    this->VisitSpan(op->span);
    return DefaultResult();
  }

  R VisitExpr_(const FunctionNode* op) override {
    this->VisitSpan(op->span);
    for (Var param : op->params) {
      this->VisitVarDef(param);
    }

    this->VisitExpr(op->body);
    return DefaultResult();
  }

  R VisitExpr_(const CallNode* op) override {
    this->VisitSpan(op->span);
    this->VisitExpr(op->op);

    for (Type ty_arg : op->type_args) {
      this->VisitType(ty_arg);
    }

    for (Expr arg : op->args) {
      this->VisitExpr(arg);
    }

    if (op->shape_) {
      this->VisitExpr(Downcast<Expr>(op->shape_.value()));
    }

    return DefaultResult();
  }

  R VisitExpr_(const IfNode* op) override {
    this->VisitSpan(op->span);
    this->VisitExpr(op->cond);
    this->VisitExpr(op->true_branch);
    this->VisitExpr(op->false_branch);
    return DefaultResult();
  }

  R VisitExpr_(const OpNode* op) override { return DefaultResult(); }

  R VisitExpr_(const TupleGetItemNode* op) override {
    this->VisitSpan(op->span);
    this->VisitExpr(op->tuple);
    return DefaultResult();
  }

  R VisitExpr_(const ShapeExprNode* op) override {
    this->VisitSpan(op->span);
    return DefaultResult();
  }

  R VisitExpr_(const RuntimeDepShapeNode* op) override {
    this->VisitSpan(op->span);
    return DefaultResult();
  }

  R VisitExpr_(const ExternFuncNode* op) override {
    this->VisitSpan(op->span);
    return DefaultResult();
  }

  R VisitExpr_(const SeqExprNode* op) override {
    this->VisitSpan(op->span);
    for (BindingBlock block : op->blocks) {
      this->VisitBindingBlock(block);
    }
    return this->VisitExpr(op->body);
  }

  R VisitType(const Type& t) { return DefaultResult(); }

  R VisitSpan(const Span& span) { return DefaultResult(); }

  virtual R VisitBinding_(const VarBindingNode* binding) {
    auto to_return = this->VisitExpr(binding->value);
    this->VisitVarDef(binding->var);
    return to_return;
  }

  virtual R VisitBinding_(const MatchShapeNode* binding) {
    auto to_return = this->VisitExpr(binding->value);
    // TODO(ziheng): should we change pattern from
    // Array<PrimExpr> to ShapeExpr?
    this->VisitExpr(ShapeExpr(binding->pattern));
    if (binding->var.defined()) {
      this->VisitVarDef(binding->var);
    }

    return to_return;
  }

  /*!
   * \brief Generic dispatcher for bindings.
   * \param binding The binding to be visited.
   */
  virtual void VisitBinding(const Binding& binding) {
    if (const auto* node = binding.as<VarBindingNode>()) {
      VisitBinding_(node);
    } else if (const auto* node = binding.as<MatchShapeNode>()) {
      VisitBinding_(node);
    } else {
      LOG(FATAL) << "TypeError: Invalid type: " << binding->GetTypeKey();
    }
  }

  virtual void VisitBindingBlock_(const BindingBlockNode* block) {
    for (Binding binding : block->bindings) {
      this->VisitBinding(binding);
    }
  }

  virtual void VisitBindingBlock_(const DataflowBlockNode* block) {
    for (Binding binding : block->bindings) {
      this->VisitBinding(binding);
    }
  }

  void VisitBindingBlock(const BindingBlock& block) {
    if (const auto* node = block.as<DataflowBlockNode>()) {
      VisitBindingBlock_(node);
    } else if (const auto* node = block.as<BindingBlockNode>()) {
      VisitBindingBlock_(node);
    } else {
      LOG(FATAL) << "TypeError: Invalid type: " << block->GetTypeKey();
    }
  }

  virtual void VisitVarDef_(const DataflowVarNode* var) {
    this->VisitSpan(var->span);

    if (var->shape_) {
      this->VisitExpr(Downcast<Expr>(var->shape_.value()));
    }
  }

  virtual void VisitVarDef_(const VarNode* var) {
    this->VisitSpan(var->span);

    if (var->shape_) {
      this->VisitExpr(Downcast<Expr>(var->shape_.value()));
    }
  }

  void VisitVarDef(const Var& var) {
    if (const auto* node = var.as<DataflowVarNode>()) {
      VisitVarDef_(node);
    } else if (const auto* node = var.as<VarNode>()) {
      VisitVarDef_(node);
    } else {
      LOG(FATAL) << "TypeError: Invalid type: " << var->GetTypeKey();
    }
  }
};

/*! \brief Discover Relax Tensors and build a set of Scope which guide TIR Stmt emission.
 *
 * This is the first pass of two and must be run before running AOTExecutorCodegen. Its
 * job is to popualte ScopeProjection with a map from Relax Expr to RealizedExpr. This visitor
 * returns
 *
 * WIP note: Another looming problem is what to do when an IfNode is encountered. Then we need
 * to either consider what's returned to be an Object somehow or assert that they return the same
 * sized Buffer so we can reuse that storage.
 */
class ScopeCollector : public DefaultExprFunctor<RealizedExpr> {
 public:
  ScopeCollector(ScopeProjection scope) : scope_{scope} {}

  RealizedExpr DefaultResult() override {
    static RealizedExpr result;
    return result;
  }

  RealizedExpr VisitExpr_(const CallNode* call_node) override {
    DefaultExprFunctor::VisitExpr_(call_node);
    if (call_node->op.as<OpNode>() != nullptr) {
      if (call_node->op == aot_op::alloc_tensor_op) {
        return scope_->Realize(GetRef<Expr>(call_node));
      }
    }

    return RealizedExpr();
  }

  RealizedExpr VisitExpr_(const VarNode* var_node) override {
    DefaultExprFunctor::VisitExpr_(var_node);

    return scope_->Lookup(GetRef<Expr>(var_node));
  }

  RealizedExpr VisitBinding_(const VarBindingNode* node) override {
    LOG(INFO) << "Visit binding: " << RelaxScriptPrinter(false, nullptr).Print(GetRef<VarBinding>(node)).str();
    RealizedExpr expr = DefaultExprFunctor::VisitBinding_(node);
    if (const CallNode* call_rhs = node->value.as<CallNode>()) {
      LOG(INFO) << "Is CallNode";
      if (const GlobalVarNode* call_op = call_rhs->op.as<GlobalVarNode>()) {
        LOG(INFO) << "Not requiring a return value for call_packed (global) " << call_op->name_hint;
        return expr;
      } else if (const ExternFuncNode* ext_func = call_rhs->op.as<ExternFuncNode>()) {
        LOG(INFO) << "Not requiring a return value for call_packed (extern) " << ext_func->global_symbol;
        return expr;
      }
    }
    LOG(INFO) << "Type: " << RelaxScriptPrinter(false, nullptr).Print(node->value->checked_type()).str();
    CHECK(expr.defined()) << "Did not resolve rhs of binding: "
                          << RelaxScriptPrinter(false, nullptr).Print(GetRef<VarBinding>(node)).str();
    LOG(INFO) << "Binding " << expr << " to " << RelaxScriptPrinter(false, nullptr).Print(node->var).str();
    scope_->Bind(expr, node->var);
    return expr;
  }

  RealizedExpr Collect(IRModule m, Function top_level) {
    return VisitExpr(top_level->body);
  }

 private:
  NameSupply name_supply_;
  ScopeProjection scope_;
};


/*! \brief Code generator for AOT executor */
class AOTExecutorCodegen : public ExprVisitor {
 protected:

  /*!
   * \brief Copy a variable to the output. This function is mainly used in edge cases
   * when we want to return an input or a parameter.
   * TODO(giuseros): we should try to avoid unnecessary copy to the output, e.g., in a
   * copy-on-write fashion.
   */
  void CopyToOutput(PrimExpr out, PrimExpr in, bool pack_input, size_t size) {
    // Define intermediate DLTensor to load/store the data
    tir::Buffer tmp_read =
        tir::decl_buffer({IntImm(DataType::UInt(64), size)}, DataType::UInt(8), "tmp_read");
    tir::Buffer tmp_write =
        tir::decl_buffer({IntImm(DataType::UInt(64), size)}, DataType::UInt(8), "tmp_write");
    te::Var loop_idx("i", DataType::Int(32));
    auto retval_i = tir::BufferLoad(tmp_read, {loop_idx});
    // Copy the variable from the input to the output
    tir::Stmt copy = tir::For(
        loop_idx, 0, tir::make_const(DataType::Int(32, 1), size, Span()), tir::ForKind::kSerial,
        tir::BufferStore(tmp_write, tir::Let(tmp_read->data, in, retval_i), {loop_idx}));
    stmts_.push_back(tir::LetStmt(tmp_write->data, out, copy));
  }

  /*!
   * Utility function to string together different arguments
   */
  template <typename... Args>
  std::string MakeString(Args const&... args) {
    std::ostringstream ss;
    using List = int[];
    (void)List{0, ((void)(ss << args), 0)...};

    return ss.str();
  }

  /*
   * Wraps a call_extern with a tvm_check_return annotation if required otherwise
   * returns the passed Call
   */
  tir::Call AddCheckReturn(tir::Call existing_call) {
    Array<PrimExpr> args = {tir::make_const(DataType::Int(32, 1), 0, Span()),
                            tir::make_const(DataType::Int(32, 1), -1, Span()), existing_call};
    return tir::Call(DataType::Int(32), tir::builtin::tvm_check_return(), args);
  }

  void VisitExpr_(const CallNode* call_node) {
    const Call& call = GetRef<Call>(call_node);
    if (call->op.as<OpNode>() != nullptr) {
      if (call->op == aot_op::alloc_tensor_op) {
        // Skip alloc_tensor; these are handled by injecting AllocateNode into the function in CreateMainFunc.
        return;
      }

      FCallPacked name = GetPackedFuncName(call);
      ICHECK(!name.empty()) << "Expect CallNode to have non-empty PackedFunc name; got " << call;


      LOG(FATAL) << "Don't know how to handle this op: " << call->op;
    }

    String name;
    if (auto* extern_func = call_node->op.as<ExternFuncNode>()) {
      name = extern_func->global_symbol;
    } else if (auto* gvar = call_node->op.as<GlobalVarNode>()) {
      // GlobalVar can be reference to a Relax function or a TIR primfunc
      name = gvar->name_hint;
    } else {
      LOG(FATAL) << "CodeGenVM does not support calls to " << call_node->op->GetTypeKey();
    }

    // Convert args.
    Array<PrimExpr> args;
    args.push_back(tir::StringImm(name));
    LOG(INFO) << "translate call_tir " << name;
    for (auto relax_arg : call->args) {
      // NOTE: for now, expect A-Normal Form. When it's clearer how to write an ExprFunctor that returns tir::Var,
      // we can use that to properly recurse here.
      auto arg = scope_->Lookup(relax_arg);
      LOG(INFO) << "translate arg " << arg;
      for (unsigned int i = 0; i < arg->NumVars(); i++) {
        args.push_back(arg->Var(i));
      }
    }

    stmts_.push_back(tir::Evaluate(AddCheckReturn(tir::Call(DataType::Int(32), tir::builtin::tvm_call_cpacked(), args))));
  }

  // Create the main PrimFunc to execute the graph. Please note that
  // the packed function calls don't pack their arguments. The AOT
  // runner function needs to be legalized by the LegalizePackedCalls pass.
  tir::PrimFunc CreateMainFunc(String mod_name, tir::Stmt body, unsigned int relay_params) {

    // Allocate the sids
    std::unordered_set<RealizedExpr, ObjectPtrHash, ObjectPtrEqual> allocated;

    for (auto kv : scope_->allocs) {
      // Only allocate sids that are needed
      if (allocated.find(kv.second) == allocated.end() && !kv.second->is_output()) {
        LOG(INFO) << "Create allocs for " << kv.second;
        for (unsigned int i = 0; i < kv.second->NumVars(); i++) {
          tir::Buffer buf = kv.second->Buffer(i);
          body = tir::Allocate(kv.second->Var(i), buf->dtype, buf->shape, tir::const_true(), body);
        }
        allocated.insert(kv.second);
      }
    }

    for (auto kv : constant_map_) {
      auto buffer_var = kv.first;
      auto dtype = DataType(kv.second->data->dtype);

      int ndim = kv.second->data->ndim;
      Array<PrimExpr> extents;

      for (int i = 0; i < ndim; i++) {
        int shape = kv.second->data->shape[i];
        extents.push_back(tir::make_const(DataType::Int(32), shape, Span()));
      }
      body = tir::AllocateConst(buffer_var, dtype, extents, kv.second->data, body);
    }

    // Define the PrimFunc attributes
    Map<String, ObjectRef> dict_attrs;
    String run_func_name = runtime::get_name_mangled(mod_name, runtime::symbol::tvm_module_main);
    dict_attrs.Set("global_symbol", run_func_name);
    dict_attrs.Set("runner_function", Bool(true));
    dict_attrs.Set(tvm::attr::kTarget, config_->host_target);

    // tir::Stmt device_activations = GenerateAllDeviceHook("Activate");
    // tir::Stmt device_deactivations = GenerateAllDeviceHook("Deactivate");
    // tir::Stmt final_body = tir::SeqStmt({device_activations, body, device_deactivations});

    // Make the PrimFunc
    return tir::PrimFunc(main_signature_, body, VoidType(), main_buffer_map_, {},
                         DictAttrs(dict_attrs));
  }

  /*!
   * \brief Access IO vars using the buffer vars and
   * not the actual var.
   */
  tir::Var GetBufferVarForIO(int index) { return main_buffer_map_[main_signature_[index]]->data; }

  /*!
   * \brief Calculate workspace sizes for PrimFuncs in the IRModule
   */
  Map<String, FunctionInfo> CalculateWorkspaceSizes(
      const IRModule& lowered_mod, const Map<String, FunctionInfo>& function_metadata) {
    Integer workspace_byte_alignment = GetModuleWorkspaceByteAlignment(lowered_mod);
    Map<String, FunctionInfo> updated_function_metadata;
    for (const auto& kv : lowered_mod->functions) {
      GlobalVar global_var = kv.first;
      BaseFunc base_func = kv.second;
      if (base_func->IsInstance<tir::PrimFuncNode>()) {
        tir::PrimFunc pfunc = Downcast<tir::PrimFunc>(base_func);
        Target tgt = pfunc->GetAttr<Target>(tvm::attr::kTarget).value();
        const auto& ws = tir::CalculateWorkspaceBytes(pfunc, workspace_byte_alignment);
        if (function_metadata.count(global_var->name_hint)) {
          updated_function_metadata.Set(global_var->name_hint,
                                        function_metadata[global_var->name_hint]);
          updated_function_metadata[global_var->name_hint]->workspace_sizes.Set(tgt, ws);
        } else {
          FunctionInfo finfo{{{tgt, ws}}, {}, {}, {{tgt, pfunc}}, {}};
          updated_function_metadata.Set(global_var->name_hint, finfo);
        }
      }
    }
    return updated_function_metadata;
  }

  /*!
   * \brief Run USMP to plan memory for lowered IRModule.
   */
  IRModule PlanMemoryWithUSMP(const IRModule& mod) {
    VLOG(1) << "Planning memory with USMP for module:" << std::endl << PrettyPrint(mod);
    Integer workspace_byte_alignment = GetModuleWorkspaceByteAlignment(mod);
    IRModule lowered_mod = mod->ShallowCopy();
    lowered_mod = tir::transform::UnifiedStaticMemoryPlanner()(lowered_mod);
    function_metadata_ = CalculateWorkspaceSizes(lowered_mod, function_metadata_);
    Optional<Array<tir::usmp::AllocatedPoolInfo>> allocated_pool_infos =
        lowered_mod->GetAttr<Array<tir::usmp::AllocatedPoolInfo>>(tvm::attr::kPoolArgs);
    backend::FunctionInfo main_func_info =
        lowered_mod->GetAttr<backend::FunctionInfo>("main_func_info").value();
    main_func_info->workspace_sizes.clear();
    if (allocated_pool_infos) {
      for (const tir::usmp::AllocatedPoolInfo& allocated_pool_info : allocated_pool_infos.value()) {
        for (const auto& tgt : allocated_pool_info->pool_info->targets) {
          VLOG(1) << "USMP requires target " << tgt->ToDebugString() << " to have pool size "
                  << allocated_pool_info->allocated_size->value;
          size_t size = allocated_pool_info->allocated_size->value;
          if (allocated_pool_info->pool_info->IsInstance<ConstantPoolInfoNode>()) {
            size += main_func_info->constant_sizes.count(tgt)
                        ? main_func_info->constant_sizes[tgt]->value
                        : 0;
            main_func_info->constant_sizes.Set(tgt, size);
          } else if (allocated_pool_info->pool_info->IsInstance<WorkspacePoolInfoNode>()) {
            size += main_func_info->workspace_sizes.count(tgt)
                        ? main_func_info->workspace_sizes[tgt]->value
                        : 0;
            main_func_info->workspace_sizes.Set(tgt, size);
          } else {
            LOG(FATAL) << "Unknown pool type: " << allocated_pool_info->pool_info->GetTypeKey();
          }
        }
      }
    }
    function_metadata_.Set(runtime::symbol::tvm_module_main, main_func_info);
    return lowered_mod;
  }

  /*!
   * \brief Run StorageRewrite to plan memory for lowered IRModule.
   */
  IRModule PlanMemoryWithStorageRewrite(const IRModule& mod) {
    Integer workspace_byte_alignment = GetModuleWorkspaceByteAlignment(mod);
    IRModule lowered_mod = mod->ShallowCopy();
    function_metadata_ = CalculateWorkspaceSizes(lowered_mod, function_metadata_);
    // Running StorageRewrite just on the main function
    tir::PrimFunc tir_main_func =
        Downcast<tir::PrimFunc>(lowered_mod->Lookup(::tvm::runtime::symbol::tvm_module_main));
    IRModule main_func_mod;
    main_func_mod->Update(lowered_mod->GetGlobalVar(::tvm::runtime::symbol::tvm_module_main),
                          tir_main_func);
    main_func_mod = tir::transform::StorageRewrite()(main_func_mod);
    lowered_mod->Update(lowered_mod->GetGlobalVar(::tvm::runtime::symbol::tvm_module_main),
                        main_func_mod->Lookup(::tvm::runtime::symbol::tvm_module_main));
    tir_main_func =
        Downcast<tir::PrimFunc>(lowered_mod->Lookup(::tvm::runtime::symbol::tvm_module_main));
    // Use the PrimFunc to calculate the workspace required to service the allocates
    Integer main_workspace_size_bytes =
        CalculateWorkspaceBytes(tir_main_func, workspace_byte_alignment);
//    backend::FunctionInfo main_func_info =
//        lowered_mod->GetAttr<backend::FunctionInfo>("main_func_info").value();
//    main_func_info->workspace_sizes.Set(config_->host_target, main_workspace_size_bytes);
//    function_metadata_.Set(runtime::symbol::tvm_module_main, main_func_info);
    return lowered_mod;
  }

  /*!
   * \brief Gets module workspace alignment from supplied executor or defaults to 16
   */
  Integer GetModuleWorkspaceByteAlignment(const IRModule& mod) {
    return Integer(16);
//    Executor executor_config = mod->GetAttr<Executor>(tvm::attr::kExecutor).value();
//    return executor_config->GetAttr<Integer>("workspace-byte-alignment").value_or(16);
  }

  /*!
   * \brief Gets module constant alignment from supplied executor or defaults to 16
   */
  Integer GetModuleConstantByteAlignment(const IRModule& mod) {
    return Integer(16);
//    Executor executor_config = mod->GetAttr<Executor>(tvm::attr::kExecutor).value();
//    return executor_config->GetAttr<Integer>("constant-byte-alignment").value_or(16);
  }

 protected:
  /*! \brief list of input/output expressions (i.e., variable passed by the user) */
  std::vector<Expr> io_expr_;
  /*! \brief input and output variables belonging to the main function signature */
  Array<tir::Var> main_signature_;
  /*! \brief input and output variables belonging to the main function signature */
  Map<tir::Var, tir::Buffer> main_buffer_map_;
  /*! \brief maps input and output variables to TensorType which describe them */
  Map<tir::Var, TensorType> io_tensor_types_;
  /*! \brief All available targets. */
  CompilationConfig config_;

  /*!
   * \brief parameters (i.e. ConstantNodes found in the graph).
   * These are take as inputs to the GraphRuntime.
   * Maps param name to a pair of storage_id and NDArray. At runtime, the storage_id can be
   * used to lookup the parameter.
   */
  std::unordered_map<std::string, runtime::NDArray> params_;
  /*! \brief mapping between expression and parameters */
  Map<Expr, String> params_by_expr_;
  std::unordered_map<const tir::Var, const ConstantNode*, ObjectPtrHash, ObjectPtrEqual>
      constant_map_;

  /*! \brief lowered funcs */
  Map<String, FunctionInfo> function_metadata_;

  /*! \brief the set of statements that make the program */
  std::vector<tir::Stmt> stmts_;
  /*! \brief the list of return sids (note that the function might return more then one output */
  std::vector<int> return_sid_;
  ScopeProjection scope_;
  /*! \brief A set of variables that are let bound. */
  std::unordered_set<Var, ObjectPtrHash, ObjectPtrEqual> let_bound_vars_;
  /*! \brief NameSupply for new var names. */
  NameSupply name_supply_;

 public:
  AOTExecutorCodegen(const Array<Target>& targets)
      : config_(transform::PassContext::Current(), targets) {}

  LoweredOutput Codegen(IRModule mod, relax::Function func, String mod_name) {
    VLOG_CONTEXT << "RELAX_AOT";

    name_supply_ = NameSupply(mod_name);

    // Post-lowering storage map for writing main func
//    AOTOnDemandAllocator final_aot_allocator;
//    final_aot_allocator.Run(lowered_main_func);
//    storage_device_map_ = final_aot_allocator.GetStorageMap();

    // TODO(@electriclilies, @jroesch, @Mousius): remove UpdateMainWorkspaceSize
//    StaticMemoryPlan memory_plan(storage_device_map_);
///    backend::FunctionInfo func_info =
//        tec::UpdateMainWorkspaceSize(lowered_mod, config_, memory_plan->expr_to_storage_info);
//    lowered_mod = WithAttr(lowered_mod, "main_func_info", func_info);

    // Create parent scope containing the I/O vars.
    scope_ = ScopeProjection(std::move(make_object<ScopeProjectionNode>()));
    for (auto input : func->params) {
      io_expr_.push_back(input);
      auto realized = RealizedExpr(input, input->name_hint(), /* use_unique_name= */ false);
      scope_->allocs.Set(input, realized);
      realized->Implement(name_supply_, /* decl_buffers= */ false);
      for (unsigned int i = 0; i < realized->NumVars(); i++) {
        main_signature_.push_back(realized->Var(i));
      }
    }

    int num_input_params = main_signature_.size();

    // Insert outputs to main func signature
    // NOTE: In Relax, I think all functions are canonicalized to a SeqExpr.
    SeqExpr seq = Downcast<SeqExpr>(func->body);

    LOG(INFO) << "Created top-level projection: " << scope_;
    ScopeCollector scope_collector{scope_};
    RealizedExpr output = scope_collector.Collect(mod, func);
    io_expr_.push_back(output->expr);

    for (auto it : scope_->allocs) {
      if (!it.second->is_implemented()) {
        LOG(INFO) << "Impl " << it.second;
        it.second->Implement(name_supply_, /* decl_buffers= */ it.second != output);
      }
    }

    for (unsigned int i = 0; i < output->NumVars(); i++) {
      main_signature_.push_back(output->Var(i));
    }

    VisitExpr(seq);

    // AoT Executor codegen works completely on TIR beyond this point, hence removing relay main
    // function and replacing it with its TIR version. We should try to make this a Pass.
    auto tir_main_func = CreateMainFunc(mod_name, tir::SeqStmt(stmts_), io_expr_.size());
    LOG(INFO) << "Created main func: " << tir_main_func;
    // Extract additional information around main TIR PrimFunc arguments
    Array<String> devices; // = ListDevices();
    const auto outputs_begin_iterator = tir_main_func->params.begin() + num_input_params;
    Array<tir::Var> inputs = Array<tir::Var>(tir_main_func->params.begin(), outputs_begin_iterator);
    Array<TensorType> input_tensor_types;
    // for (auto i : inputs) {
    //   input_tensor_types.push_back(io_tensor_types_[i]);
    // }
    Array<tir::Var> outputs =
        Array<tir::Var>(outputs_begin_iterator, tir_main_func->params.end()); //main_func_params_end_iterator - devices.size());
    size_t num_outputs = outputs.size();

    mod->Update(GlobalVar(::tvm::runtime::symbol::tvm_module_main), tir_main_func);
    // Parallel for loops are not supported in AoT codegen.
    mod = tir::transform::ConvertForLoopsToSerial()(mod);

    transform::PassContext pass_ctx = transform::PassContext::Current();
    bool enable_usmp = pass_ctx->GetConfig<Bool>(kUSMPEnableOption, Bool(false)).value();
    if (enable_usmp) {
      mod = PlanMemoryWithUSMP(mod);
    } else {
      mod = PlanMemoryWithStorageRewrite(mod);
    }
    LoweredOutput ret;
    ret.function_metadata = std::move(function_metadata_);

    // Legalize AOT if needed. This means that all the packed calls
    // need to be wrapped in TVMValues (unless unpacked_api is set)
//   if (call_type_ == CallType::kCPacked || call_type_ == CallType::kPacked) {
     auto pack_calls = tir::transform::LegalizePackedCalls();
     mod = pack_calls(mod);
//   }

    // Collect any runtime modules generated by external codegen.
    ret.external_mods =
        mod->GetAttr<Array<tvm::runtime::Module>>(tvm::attr::kExternalMods).value_or({});

    // This is the point where we separate the functions in the module by target
    VLOG(1) << "lowered module:" << std::endl << PrettyPrint(mod);
//    ret.lowered_funcs = tec::GetPerTargetModules(mod);
    // VLOG(1) << "per-target modules:";
    // for (const auto& kv : ret.lowered_funcs) {
    //   VLOG(1) << "target:" << std::endl
    //           << kv.first->ToDebugString() << std::endl
    //           << "maps to:" << std::endl
    //           << PrettyPrint(kv.second);
    // }

    // Extract USMP metadata to pass onto metadata sources
    Map<tir::Var, tir::usmp::AllocatedPoolInfo> pool_var_info;
    std::vector<tir::Var> pool_vars;
    tir_main_func =
        Downcast<tir::PrimFunc>(mod->Lookup(::tvm::runtime::symbol::tvm_module_main));
    Optional<Array<tir::usmp::AllocatedPoolInfo>> allocated_pool_infos =
        tir_main_func->GetAttr<Array<tir::usmp::AllocatedPoolInfo>>(tvm::attr::kPoolArgs);
    if (allocated_pool_infos) {
      for (const tir::usmp::AllocatedPoolInfo& allocated_pool_info : allocated_pool_infos.value()) {
        int pool_var_index = allocated_pool_info->pool_var_idx.value()->value;
        pool_vars.push_back(tir_main_func->params[pool_var_index]);
        pool_var_info.Set(tir_main_func->params[pool_var_index], allocated_pool_info);
      }
    }
    Map<String, tir::usmp::PoolAllocation> io_pool_allocations =
        mod
            ->GetAttr<Map<String, tir::usmp::PoolAllocation>>(tvm::attr::kIOTensorPoolAllocations)
            .value_or({});

    std::vector<String> output_var_names;
    if (auto opt = func->GetAttr<Array<String>>("output_tensor_names")) {
      Array<String> output_tensor_names = opt.value();
      for (size_t i = 0; i < output_tensor_names.size(); ++i) {
        output_var_names.push_back(output_tensor_names[i]);
      }
    }

    // If output names have not been specified then generate default output names
    if (output_var_names.size() == 0) {
      if (return_sid_.size() == 1) {
        output_var_names.push_back(String("output"));
      } else {
        for (size_t i = 0; i < num_outputs; ++i) {
          output_var_names.push_back(String("output" + std::to_string(i)));
        }
      }
    }

    Array<TensorType> output_tensor_types{}; //final_aot_allocator.GetReturnTtypes()};

    String interface_api{"packed"};
    Bool unpacked_api{false};

    ret.metadata = relay::backend::ExecutorCodegenMetadata(
        inputs, input_tensor_types, output_var_names, output_tensor_types, pool_vars, devices,
        runtime::kTvmExecutorAot, mod_name, interface_api, unpacked_api,
        GetModuleWorkspaceByteAlignment(mod), GetModuleConstantByteAlignment(mod), pool_var_info,
        io_pool_allocations);
    return ret;
  }

  /*!
   * \brief Get list of devices found
   * \return List of devices
   */
  Array<String> ListDevices() {
    std::vector<String> device_names(0); //devices_.size());
    // std::transform(devices_.begin(), devices_.end(), device_names.begin(),
    //                [](const auto& it) -> String { return it.first; });
    return device_names;
  }
};  // namespace backend


class AOTExecutorCodegenModule : public runtime::ModuleNode {
 public:
  AOTExecutorCodegenModule() {}
  virtual PackedFunc GetFunction(const std::string& name, const ObjectPtr<Object>& sptr_to_self) {
    if (name == "init") {
      return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
        ICHECK_EQ(args.num_args, 1) << "The expected of arguments are: "
                                    << "runtime::Module mod and Array<Target> targets";
//        void* mod = args[0];
        Array<Target> targets = args[0];
        init(targets);
      });
    } else if (name == "codegen") {
      return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
        IRModule mod = args[0];
        Function func = args[1];
        String mod_name = args[2];
        this->output_ = this->codegen_->Codegen(mod, func, mod_name);
      });
    } else if (name == "list_params_name") {
      return PackedFunc(
          [sptr_to_self, this](TVMArgs args, TVMRetValue* rv) { *rv = list_params_name(); });
    } else if (name == "get_param_by_name") {
      return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
        String key = args[0];
        *rv = get_param_by_name(key);
      });
    } else if (name == "get_irmodule") {
      return PackedFunc(
          [sptr_to_self, this](TVMArgs args, TVMRetValue* rv) { *rv = get_irmodule(); });
    } else if (name == "get_external_modules") {
      return PackedFunc(
          [sptr_to_self, this](TVMArgs args, TVMRetValue* rv) { *rv = get_external_modules(); });
    } else if (name == "get_function_metadata") {
      return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
        *rv = this->output_.function_metadata;
      });
    } else if (name == "get_devices") {
      return PackedFunc([sptr_to_self, this](TVMArgs args, TVMRetValue* rv) {
        *rv = this->codegen_->ListDevices();
      });
    } else if (name == "get_executor_codegen_metadata") {
      return PackedFunc(
          [sptr_to_self, this](TVMArgs args, TVMRetValue* rv) { *rv = output_.metadata; });
    } else {
      return PackedFunc([](TVMArgs args, TVMRetValue* rv) {});
    }
  }

  const char* type_key() const final { return "RelayAotExecutorCodegenModule"; }

 private:
  void init(const Array<Target>& targets) {
    codegen_ = std::make_shared<AOTExecutorCodegen>(targets);
  }

  Array<runtime::String> list_params_name() {
    Array<runtime::String> ret;
    for (const auto& kv : this->output_.params) {
      ret.push_back(kv.first);
    }
    return ret;
  }

  runtime::NDArray get_param_by_name(String key) {
    auto it = this->output_.params.find(key);
    CHECK(it != this->output_.params.end()) << "no such parameter " << key;
    return (*it).second;
  }

  Array<tvm::runtime::Module> get_external_modules() { return output_.external_mods; }

  Map<Target, IRModule> get_irmodule() { return this->output_.lowered_funcs; }

  std::shared_ptr<AOTExecutorCodegen> codegen_;
  LoweredOutput output_;
};


TVM_REGISTER_GLOBAL("relax.AOTExecutorCodegen")
.set_body([](TVMArgs args, TVMRetValue* rv) {
  *rv = tvm::runtime::Module(std::move(make_object<AOTExecutorCodegenModule>()));
});


}  // namespace backend
}  // namespace relax
}  // namespace tvm
