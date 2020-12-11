..  Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

..    http://www.apache.org/licenses/LICENSE-2.0

..  Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.

.. _tensor_expression:

Tensor Expression Language
==========================

Initially, TVM represents models in an Intermediate Representation called **Tensor Expression**.
Models must be lowered to Tensor Expression describes a computation without specifying how it's
accomplished.

The Compute Function
~~~~~~~~~~~~~~~~~~~~~

Tensor Expression operators split the definition of a model into two pieces:
* **Compute Function**: describes the computation, element-wise, typically as a Python lambda
  function.
* **Schedule**: describes *how to perform* the computation, including loop structure, strides,
  unrolling and any acceleration performed by intrinsics using tensorization or vectorization.

These tutorials

.. toctree::
.. include:: ./tutorials/index.rst
