# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

"""Defines metadata used at runtime, plus a C and C++ code-generator.

This module is intended to be used two ways:
 1. As part of tvm.runtime package in a TVM installation.
 2. As a build-time generator to generate C and C++ definitions of the metadata passed between
    the compiler and runtime.
"""

import argparse
import contextlib
import os
import pathlib
import sys
import textwrap
import typing
from typing import List, Optional, Union


if __name__ == "__main__":
    class DataType:
        pass

    PythonType = Union[int, float, DataType, List, "MetadataBase"]

    class MetadataBase:
        _FIELDS : Optional[List[PythonType]] = None

else:
    from . import object
    from .._ffi import DataType

    class MetadataBase(object.Object):

        def __init__(self, handle):
            pass

# Start of Metadata -->

class ParameterInfo(MetadataBase):

    _FIELDS = [
        ("relay_name_hint", str),
        ("tir_name_hint", str),
        ("shape", List[int]),
        ("dtype", DataType),
    ]


class FunctionInfo(MetadataBase):

    _FIELDS = [
        ("function_name", str),
        ("params", List[ParameterInfo]),
        ("num_inputs", int),
    ]


class Metadata(MetadataBase):

    _FIELDS = [
        ("version", int),
        ("functions", List[FunctionInfo]),
        ("module_name", str),
        ("target", str),
    ]


# <-- End of Metadata


class MetadataFields:

    @classmethod
    def from_metadata(cls, obj):
        return cls(**dict(obj._FIELDS))

    def __init__(self, **kw):
        self._fields = tuple([MetadataField(field_name, python_type)
                              for field_name, python_type
                              in kw.items()])
        all_arrays = [f for f in self._fields if f.type.is_array]
        array_count_field_names = set(f"num_{f.name}" for f in all_arrays)
        illegal_fields = [f for f in self._fields if f.name in array_count_field_names]
        if illegal_fields:
            raise ValueError(
                "Illegal field names (arrarys auto-populate such a field): "
                f"{', '.join(f.name for f in illegal_fields)}")

    def __len__(self):
        return len(self._fields)

    def __getitem__(self, i):
        if isinstance(i, str):
            for f in self._fields:
                if f.name == i:
                    return f

            raise KeyError(f"no such field {i}")

        return self._fields[i]

    def __iter__(self):
        return iter(self._fields)


class MetadataType:
    """Collects utility functions for reasoning about field types."""

    C_TYPE_BY_PYTHON_TYPE = {
        int: "int64_t",
        float: "double",
        DataType: "DLDataType",
    }

    def __init__(self, python_type : PythonType, is_element_type : bool = False):
        self.python_type = python_type
        self.is_element_type = is_element_type

    @property
    def is_object(self) -> bool:
        return issubclass(self.python_type, MetadataBase)

    @property
    def is_array(self) -> bool:
#        print("TYPE", dir(self.python_type), getattr(self.python_type, '__origin__', None))
        return getattr(self.python_type, '__origin__', None) is typing.List

    @property
    def element_type(self) -> "MetadataType":
        if not self.is_array:
            raise TypeError("type {self.python_type} is not list")

        return MetadataType(self.python_type.__args__[0], is_element_type=True)

    @property
    def c_type(self) -> str:
        if self.python_type in self.C_TYPE_BY_PYTHON_TYPE:
            return self.C_TYPE_BY_PYTHON_TYPE[self.python_type]
        elif self.python_type is str:
            return "const char*"
        elif self.is_array:
            return f"{self.element_type.c_type}*"
        elif self.is_object:
            if self.is_element_type:
                return f"const struct TVM{self.python_type.__name__}"
            else:
                return f"const struct TVM{self.python_type.__name__}*"
        else:
            raise TypeError(f"unknown type {self.python_type}")

    @property
    def cpp_type(self) -> str:
        if self.python_type in (int, float):
            return self.C_TYPE_BY_PYTHON_TYPE[self.python_type]
        elif self.python_type is str:
            return "::std::string"
        elif self.python_type is DataType:
            return "::tvm::runtime::DataType"
        elif self.is_array:
            return f"::tvm::support::Span<{self.element_type.cpp_type}, {self.element_type.cpp_type}>"
        elif self.is_object:
            if self.is_element_type:
                return self.python_type.__name__
            else:
                return f"const {self.python_type.__name__}*"
        else:
          raise TypeError(f"unknown type {self.python_type}")

    @property
    def cpp_builder_type(self) -> str:
        if self.is_primitive:
            return self.cpp_type
        elif self.python_type in (str, DataType):
            return f"const {self.cpp_type}&"
        elif self.is_array:
            return f"const ::std::vector<{self.element_type.cpp_builder_type}>&"
        elif self.is_object:
            return self.python_type.__name__
        else:
            raise TypeError(f"field {field.name}: unknown type {self.python_type}")

    @property
    def is_primitive(self) -> bool:
        return self.python_type in (int, float, DataType)

    @property
    def in_memory_storage_type(self) -> str:
        if self.is_primitive:
            return self.C_TYPE_BY_PYTHON_TYPE[python_type]
        elif self.python_type is str:
            return "::std::string"
        elif self.is_array:
            return f"::std::unique_ptr<{self.c_type}>"
        elif self.is_object:
            return "{self.python_type.__name__}"  # Ref class
        else:
            raise TypeError(f"field {field.name}: unknown type {self.python_type}")


class MetadataField:
    """Collects utility functions for reasoning about fields."""

    def __init__(self, name : str, python_type : PythonType):
        self.name = name
        self.type = MetadataType(python_type)


class Writer:

    class Scope:
        def __init__(self, writer, begin, end, scope_indent):
            self.writer = writer
            self.begin = begin
            self.end = end
            self.scope_indent = scope_indent

        def __getattr__(self, key):
            return getattr(self.writer, key)

        def __enter__(self):
            self.writer.write(self.begin)
            self.writer.enter_scope(self.scope_indent)
            return self

        def __exit__(self, exc_type, exc_value, exc_traceback):
            self.writer.exit_scope(self.scope_indent)
            self.writer.write(self.end)

    def __init__(self, path : pathlib.Path, scope_indent : int=2):
        self.path = path
        self.fd = None
        self.indent = 0
        self.scope_indent = scope_indent

    def __enter__(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.fd = open(self.path, "w")
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        if self.fd:
            self.fd.close()

    def scope(self, begin, end, scope_indent=None):
        return self.Scope(self, begin, end, scope_indent if scope_indent is not None else self.scope_indent)

    def enter_scope(self, indent=None):
        self.indent += (indent if indent is not None else self.scope_indent)

    def exit_scope(self, indent=None):
        self.indent -= (indent if indent is not None else self.scope_indent)

    def write_indent(self, adjust=0):
        self.fd.write(" " * (self.indent + adjust))

    def write(self, data, adjust=0):
        line_break_pos = data.find("\n")
        while line_break_pos != -1:
            self.write_indent(adjust)
            self.fd.write(data[:line_break_pos + 1])
            data = data[line_break_pos + 1:]
            line_break_pos = data.find("\n")

        if data:
            self.write_indent(adjust)
            self.fd.write(data)
            self.fd.write("\n")


def generate_struct(obj, writer):
    with writer.scope(f"\nstruct TVM{obj.__name__} {{", "};"):
        for field in MetadataFields.from_metadata(obj):
            writer.write(f"{field.type.c_type} {field.name};")

            if field.type.is_array:
                writer.write(f"int64_t num_{field.name};")


def generate_class(obj, header, impl):
    with header.scope(f"\nclass {obj.__name__}Node : public MetadataBaseNode {{", "};") as cls:
        cls.write(f"public:", adjust=-1)
        cls.write(f"{obj.__name__}Node(const struct ::TVM{obj.__name__}* data) : data_{{data}} {{}}")
        metadata_fields = MetadataFields.from_metadata(obj)
        for field in metadata_fields:
            if field.type.is_array:
                element_type = field.type.element_type
                if element_type.is_primitive:
                    with cls.scope(f"inline {field.type.cpp_type} {field.name}() const {{", "}") as func:
                      func.write(f"return {field.type.cpp_type}(data_->{field.name}, data_->{field.name} + data_->num_{field.name});")
                else:
                    cls.write(f"ArrayAccessor<{field.type.c_type}, {element_type.cpp_type}> {field.name}();")
                    with impl.scope(f"ArrayAccessor<{field.type.c_type}, {element_type.cpp_type}> {obj.__name__}Node::{field.name}() {{", "}") as func:
                        func.write(
                            f"if ({field.name}_refs_.get() == nullptr) {{ {field.name}_refs_.reset(new ::std::vector<{element_type.cpp_type}>()); }}")
                        func.write(
                            f"return ArrayAccessor<{field.type.c_type}, {element_type.cpp_type}>(&data_->{field.name}, data_->num_{field.name}, {field.name}_refs_);")
            else:
                cls.write(f"inline {field.type.cpp_type} {field.name}() const {{ return {field.type.cpp_type}(data_->{field.name}); }}")

        cls.write(f"const struct ::TVM{obj.__name__}* data() const {{ return data_; }}")
        cls.write(f"TVM_DECLARE_FINAL_OBJECT_INFO({obj.__name__}Node, MetadataBaseNode);")

        cls.write(f"private:", adjust=-1)
        cls.write(f"const struct ::TVM{obj.__name__}* data_;")

        for field in metadata_fields:
            if field.type.is_array and not field.type.element_type.is_primitive:
                cls.write(f"::std::shared_ptr<::std::vector<{field.type.element_type.cpp_type}>> {field.name}_refs_;")

    with header.scope(f"\nclass {obj.__name__} : public MetadataBase {{", "};") as cls:
        cls.write("public:", adjust=-1)
        cls.write(f"{obj.__name__}(const struct ::TVM{obj.__name__}* data);")
        cls.write(f"TVM_DEFINE_OBJECT_REF_METHODS({obj.__name__}, MetadataBase, {obj.__name__}Node);")

    impl.write(f"{obj.__name__}::{obj.__name__}(const struct ::TVM{obj.__name__}* data) :")
    impl.write(f"MetadataBase{{make_object<{obj.__name__}Node>(data)}} {{}}", adjust=4)


def generate_in_memory_class(obj : MetadataBase, header : Writer, impl : Writer):
    cls_def = (
        f"\nclass InMemory{obj.__name__}Node : public ::tvm::runtime::metadata::{obj.__name__}Node {{")
    with header.scope(cls_def, "};") as cls:
        cls.write(f"public:", adjust=-1)
        cls.write(f"InMemory{obj.__name__}Node(")
        metadata_fields = MetadataFields.from_metadata(obj)
        for i, field in enumerate(metadata_fields):
            cls.write(f"{field.type.cpp_builder_type} {field.name}{',' if i < len(obj._FIELDS) - 1 else ''}", adjust=6)
        cls.write(") : ", adjust=4)

        # setup non-primitive storage.
        for i, field in enumerate(metadata_fields):
            if field.type.is_primitive:
                continue

            if field.type.is_array:
                cls.write(f"{field.name}_{{new {field.type.element_type.c_type}[{field.name}.size()]()}}{', ' if i < len(metadata_fields) - 1 else ''}", adjust=4)
            else:
                cls.write(f"{field.name}_{{{field.name}}}{', ' if i < len(metadata_fields) - 1 else ''}", adjust=4)

        with cls.scope("    storage_{", "    },", scope_indent=8) as storage_init:
            for i, field in enumerate(metadata_fields):
                if field.type.is_primitive:
                    storage_init.write(f"{field.name}_{', ' if i < len(metadata_fields) - 1 else ''}")
                elif field.type.is_array or field.type.is_object:  # same accesor method
                    storage_init.write(f"{field.name}_.get(){', ' if i < len(metadata_fields) - 1 else ''}")
                elif field.type.python_type is str:
                    storage_init.write(f"{field.name}_.c_str(){', ' if i < len(metadata_fields) - 1 else ''}")
                else:
                    assert False
        with cls.scope(f"    {obj.__name__}Node{{&storage_}} {{", "}") as init_scope:
            for i, field in enumerate(metadata_fields):
                if field.type.is_array:
                    with init_scope.scope(f"for (int i = 0; i < {field.name}.size(); ++i) {{", "}") as for_scope:
                        if field.type.element_type.is_primitive:
                            for_scope.write(f"{field.name}_[i] = {field.name}_[i];")
                        elif field.type.element_type.python_type is str:
                            for_scope.write(f"{field.name}_[i] = {field.name}_[i].c_str();")
                        elif field.type.element_type.is_object:
                            for_scope.write(f"{field.name}_[i] = {field.name}_[i]->data();")
                        elif field_type.element_type.is_array:
                            raise TypeError(f"field {field.name}: array of array not supported")
                        else:
                            raise TypeError(f"field {field.name}: don't know how to handle type {field.type.python_type}")

        with cls.scope("\nvoid VisitAttrs(AttrVisitor* v) {", "}") as visit:
            for i, field in enumerate(metadata_fields):
                if field.type.python_type in (int, float):
                    visit.write(f'v->Visit("{field.name}", &storage_->{field.name});')
                elif field.type.python_type is DataType:
                    visit.write(f"::tvm::runtime::DataType {field.name}_dtype{{{field.name}}};")
                    visit.write(f'v->Visit("{field.name}", {field.name}_dtype);')
                    visit.write(f"storage_->{field.name} = {field.name}_dtype;")
                elif field.type.is_array:
                    # visit.write(f"::std::vector<{field.prim_expr_ref_type}> {field.name}_vector;")
                    # visit.write(f"{field.name}_vector.reserve(storage_.num_{field.name});")
                    # with visit.scope(f"for (int64_t i = 0; i < storage_.num_{field.name}; ++i) {{", "}") as list_for:
                    #     element_type = field.type.element_type
                    #     if element_type.is_primitive:
                    #         visit.write(f"      {field.name}_vector[i].emplace_back({field.prim_expr_ref_type}{{storage_.{field.name}}});")
                    #     elif element_type.is_array:
                    #         raise TypeError("list of list not supported")
                    #     elif element_type.is_object:
                    #         visit.write(f"      {field.name}_vector[i].emplace_back({field.prim_expr_ref_type}{{storage_.{field.name}}});")
                    #     else:
                    #         raise TypeError(f"no such type: {element_type}")
#                    visit.write(f"auto {field.name}_accessor = {field.name}();");
#                    visit.write(f"Array<{field.prim_expr_ref_type}> {field.name}_array{{{field.name}_accessor.begin(), {field.name}_accessor.end()}};");
                    visit.write(f'v->Visit("{field.name}", &{field.name}_array);')
                elif field.type.is_object:
                    visit.write(f'v->Visit("{field.name}", &{field.name}_);')
                elif field.type.python_type is str:
                    visit.write(f'v->Visit("{field.name}", &{field.name}_);')
                else:
                    raise TypeError(f"Don't know how to codegen field {field.name} with type {field.type.python_type}")

        cls.write("\nprivate:", adjust=-1)
        cls.write(f"struct ::TVM{obj.__name__} storage_;")

        # define non-primitive storage.
        for i, field in enumerate(metadata_fields):
            if field.type.is_primitive:
                continue

            cls.write(f"{field.type.in_memory_storage_type} {field.name}_;")


def path_arg(s):
    p = pathlib.Path(s)
    p = p.resolve()
    if not p.exists:
        raise argparse.ArgumentTypeError(f"file does not exist: {p}")
    return p


def header_guard_scope(writer, file_path, generated_toplevel_dir):
    relpath = file_path.relative_to(generated_toplevel_dir)
    define = f"TVM_{str(relpath).replace(os.path.sep, '_').replace('.', '_').upper()}"
    return writer.scope(f"#ifndef {define}\n#define {define}\n", f"\n#endif  // {define}", scope_indent=0)


def namespace_scope(writer, namespaces, include_c_guard=False):
    start = "\n".join(f"namespace {ns} {{" for ns in namespaces) + "\n\n"
    end = "\n" + "\n".join(f"}}  // namespace {ns}" for ns in reversed(namespaces))
    if include_c_guard:
        start = f"#ifdef __cplusplus\n{start}#endif  // defined(__cplusplus))"
        start = f"#ifdef __cplusplus\n{end}#endif  // defined(__cplusplus))"

    return writer.scope(start, end, scope_indent=0)


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime-header", required=True, type=path_arg, help="Path to the libtvm_runtime.so header file.")
    parser.add_argument("--runtime-impl", required=True, type=path_arg, help="Path to the libtvm_runtime.so implementation file.")
    parser.add_argument("--compiler-header", required=True, type=path_arg, help="Path to the libtvm.so header file.")
    parser.add_argument("--compiler-impl", required=True, type=path_arg, help="Path to the libtvm.so header file.")
    parser.add_argument("--generated-toplevel-dir", required=True, type=path_arg,
                        help=("Path to a dir which should contain all other paths. Include paths and header guards"
                              "will be computed relative to this directory."))
    args = parser.parse_args()

    to_visit = [Metadata]
    seen = set(to_visit)
    traversal_order = []
    def _add_obj(o):
        if issubclass(o, MetadataBase) and o not in seen:
            to_visit.append(o)

    while to_visit:
        obj = to_visit.pop()
        traversal_order.append(obj)

        for field in MetadataFields.from_metadata(obj):
            if field.type.is_object:
                _add_obj(field.type.python_type)
            elif field.type.is_array:
                _add_obj(field.type.element_type.python_type)

    with contextlib.ExitStack() as rt_stack:
        rt_header = rt_stack.enter_context(Writer(args.runtime_header))
        rt_impl = rt_stack.enter_context(Writer(args.runtime_impl))
        rt_header.write("// AUTOGENERATED DO NOT EDIT")
        guard_scope = rt_stack.enter_context(
            header_guard_scope(rt_header, args.runtime_header, args.generated_toplevel_dir))
        guard_scope.write("#include <inttypes.h>")
        guard_scope.write("#include <tvm/runtime/c_runtime_api.h>")
        guard_scope.write("#include <tvm/support/span.h>")
        guard_scope.write(textwrap.dedent("""\
            #ifdef __cplusplus
            extern "C" {
            #endif
            """))
        for obj in traversal_order:
            generate_struct(obj, guard_scope)

        guard_scope.write(textwrap.dedent("""\
            #ifdef __cplusplus
            }  // extern "C"
            """))

        guard_scope.write("#include <tvm/runtime/object.h>")
        with namespace_scope(guard_scope, ["tvm", "runtime", "metadata"]) as header_ns_scope:
            rt_impl.write(f'#include <tvm/runtime/metadata.h>')
            impl_ns_scope = rt_stack.enter_context(
                namespace_scope(rt_impl, ["tvm", "runtime", "metadata"]))

            for obj in traversal_order:
                header_ns_scope.write(f"class {obj.__name__};")

            for obj in traversal_order:
                generate_class(obj, header_ns_scope, impl_ns_scope)

        rt_header.write("#endif  // defined(__cplusplus)")

    with contextlib.ExitStack() as cc_stack:
        cc_header = cc_stack.enter_context(Writer(args.compiler_header))
        cc_impl = cc_stack.enter_context(Writer(args.compiler_impl))
        cc_header.write("// AUTOGENERATED DO NOT EDIT")

        header_ns_scope = cc_stack.enter_context(
            namespace_scope(
                cc_stack.enter_context(
                    header_guard_scope(cc_header, args.compiler_header, args.generated_toplevel_dir)),
                ["tvm", "runtime", "metadata"]))

        cc_impl.write(f'#include "{args.compiler_header.name}"')
        impl_ns_scope = cc_stack.enter_context(
            namespace_scope(cc_impl, ["tvm", "target", "metadata"]))

        for obj in traversal_order:
            generate_in_memory_class(obj, header_ns_scope, impl_ns_scope)


if __name__ == "__main__":
    main(sys.argv)
