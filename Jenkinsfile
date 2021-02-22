#!groovy
// -*- mode: groovy -*-

// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// Jenkins pipeline
// See documents at https://jenkins.io/doc/book/pipeline/jenkinsfile/

// Docker env used for testing
// Different image may have different version tag
// because some of them are more stable than anoter.
//
// Docker images are maintained by PMC, cached in dockerhub
// and remains relatively stable over the time.
// Flow for upgrading docker env(need commiter)
//
// - Send PR to upgrade build script in the repo
// - Build the new docker image
// - Tag the docker image with a new version and push to a binary cache.
// - Update the version in the Jenkinsfile, send a PR
// - Fix any issues wrt to the new image version in the PR
// - Merge the PR and now we are in new version
// - Tag the new version as the lates
// - Periodically cleanup the old versions on local workers
//

// Hashtag in the source to build current CI docker builds
//
//

// NOTE: these lines are scanned by docker/dev_common.sh. Please update the regex as needed. -->
ci_lint = "tlcpack/ci-lint:v0.62"
ci_gpu = "tlcpack/ci-gpu:v0.72"
ci_cpu = "tlcpack/ci-cpu:v0.72-t0"
ci_wasm = "tlcpack/ci-wasm:v0.70"
ci_i386 = "tlcpack/ci-i386:v0.72-t0"
ci_qemu = "tlcpack/ci-qemu:v0.01"
ci_arm = "tlcpack/ci-arm:v0.01"
// <--- End of regex-scanned config.

// tvm libraries
tvm_runtime = "build/libtvm_runtime.so, build/config.cmake"
tvm_lib = "build/libtvm.so, " + tvm_runtime
// LLVM upstream lib
tvm_multilib = "build/libtvm.so, " +
               "build/libvta_tsim.so, " +
               "build/libvta_fsim.so, " +
               tvm_runtime

// command to start a docker container
docker_run = 'docker/bash.sh'
// timeout in minutes
max_time = 240

def per_exec_ws(folder) {
  return "workspace/exec_${env.EXECUTOR_NUMBER}/" + folder
}

// initialize source codes
def init_git() {
  // Add more info about job node
  sh """
     echo "INFO: NODE_NAME=${NODE_NAME} EXECUTOR_NUMBER=${EXECUTOR_NUMBER}"
     """
  checkout scm
  retry(5) {
    timeout(time: 2, unit: 'MINUTES') {
      sh 'git submodule update --init -f'
    }
  }
}

def init_git_win() {
    checkout scm
    retry(5) {
        timeout(time: 2, unit: 'MINUTES') {
            bat 'git submodule update --init -f'
        }
    }
}

def cancel_previous_build() {
    // cancel previous build if it is not on main.
    if (env.BRANCH_NAME != "main") {
        def buildNumber = env.BUILD_NUMBER as int
        // Milestone API allows us to cancel previous build
        // with the same milestone number
        if (buildNumber > 1) milestone(buildNumber - 1)
        milestone(buildNumber)
    }
}

cancel_previous_build()

stage("Sanity Check") {
  timeout(time: max_time, unit: 'MINUTES') {
    node('CPU') {
      ws(per_exec_ws("tvm/sanity")) {
        init_git()
        sh "${docker_run} ${ci_lint}  ./tests/scripts/task_lint.sh"
      }
    }
  }
}

// Run make. First try to do an incremental make from a previous workspace in hope to
// accelerate the compilation. If something wrong, clean the workspace and then
// build from scratch.
def make(docker_type, path, make_flag) {
  timeout(time: max_time, unit: 'MINUTES') {
    try {
      sh "${docker_run} ${docker_type} ./tests/scripts/task_build.sh ${path} ${make_flag}"
      // always run cpp test when build
      sh "${docker_run} ${docker_type} ./tests/scripts/task_cpp_unittest.sh"
    } catch (hudson.AbortException ae) {
      // script exited due to user abort, directly throw instead of retry
      if (ae.getMessage().contains('script returned exit code 143')) {
        throw ae
      }
      echo 'Incremental compilation failed. Fall back to build from scratch'
      sh "${docker_run} ${docker_type} ./tests/scripts/task_clean.sh ${path}"
      sh "${docker_run} ${docker_type} ./tests/scripts/task_build.sh ${path} ${make_flag}"
      sh "${docker_run} ${docker_type} ./tests/scripts/task_cpp_unittest.sh"
    }
  }
}

// pack libraries for later use
def pack_lib(name, libs) {
  sh """
     echo "Packing ${libs} into ${name}"
     echo ${libs} | sed -e 's/,/ /g' | xargs md5sum
     """
  stash includes: libs, name: name
}


// unpack libraries saved before
def unpack_lib(name, libs) {
  unstash name
  sh """
     echo "Unpacked ${libs} from ${name}"
     echo ${libs} | sed -e 's/,/ /g' | xargs md5sum
     """
}

stage('Build') {
  'BUILD: CPU': {
    node('CPU') {
      ws(per_exec_ws("tvm/build-cpu")) {
        init_git()
        sh "${docker_run} ${ci_cpu} ./tests/scripts/task_config_build_cpu.sh"
        make(ci_cpu, 'build', '-j2')
        pack_lib('cpu', tvm_multilib)
        timeout(time: max_time, unit: 'MINUTES') {
          sh "${docker_run} ${ci_cpu} ./tests/scripts/task_ci_setup.sh"
          sh "${docker_run} ${ci_cpu} ./tests/scripts/task_python_unittest.sh"
          junit "build/pytest-results/*.xml"
        }
      }
    }
  }
}
