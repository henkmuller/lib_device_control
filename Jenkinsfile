@Library('xmos_jenkins_shared_library@v0.32.0') _

getApproval()

def runningOn(machine) {
    println "Stage running on:"
    println machine
}

pipeline {
  agent none

  options {
    timestamps()
    buildDiscarder(xmosDiscardBuildSettings(onlyArtifacts=false))
  }

  environment {
    REPO = 'lib_device_control'
    VIEW = getViewName(REPO)
  }
  parameters {
    string(
      name: 'TOOLS_VERSION',
        defaultValue: '15.3.0',
        description: 'The XTC tools version'
    )
  }
  stages {
    stage('Cross-platform builds and tests') {
      parallel {
        stage('Library checks, tests and Mac x86_64 host builds') {
          agent {
            label 'x86_64&&macOS'
          }
          stages {
            stage('Get view') {
              steps {
                xcorePrepareSandbox("${VIEW}", "${REPO}")
              }
            }
            stage('Library checks') {
              steps {
                xcoreLibraryChecks("${REPO}")
              }
            }
            stage('Tests') {
              steps {
                runXmostest("${REPO}", 'tests')
              }
            }
            stage('Host builds') {
              steps {
                script {
                  hostAppList = ['usb/host', 'xscope/host']
                }
                script {
                  withTools(params.TOOLS_VERSION) {
                    hostAppList.each { app ->
                      dir("${REPO}/examples/${app}") {
                        sh "cmake -B build"
                        sh "make -C build"
                      }
                    }
                  }
                }
              }
            }
            stage('xCORE builds') {
              steps {
                script {
                  appList = ['i2c', 'i2c/host_xcore', 'spi', 'usb', 'xscope']
                }
                script {
                  appList.each { app ->
                    dir("${REPO}/examples/${app}") {
                      withTools(params.TOOLS_VERSION) {
                        withVenv {
                          sh 'cmake -G "Unix Makefiles" -B build'
                          sh "xmake -C build"
                        }
                      }
                    }
                  }
                }
              }
            }
            stage('Build documentation') {
              steps {
                dir("${REPO}/${REPO}") {
                  runXdoc('doc')
                }
              }
           }
          }
        }
        stage('RPI Host builds') {
          agent {
            label 'armv7l&&raspian'
          }
          stages {
            stage('Build') {
              steps {
                runningOn(env.NODE_NAME)
                checkout scm

                script {
                  hostAppList = ['i2c/host_rpi', 'spi/host']
                  hostAppList.each { app ->
                    dir("examples/${app}") {
                      sh "cmake -B build"
                      sh "make -C build"
                    }
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        } // RPI host builds

        stage('Linux x86_64 host builds') {
          agent {
            label 'linux&&x86_64'
          }
          stages {
            stage('Build') {
              steps {
                runningOn(env.NODE_NAME)
                checkout scm

                script {
                  withTools(params.TOOLS_VERSION) {
                      hostAppList = ['usb/host', 'xscope/host']
                      hostAppList.each { app ->
                        dir("examples/${app}") {
                          sh "cmake -B build"
                          sh "make -C build"
                        }
                    }
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        } // Linux x86_64 host suilds

        stage('Mac arm64 host builds') {
          agent {
            label 'macos&&arm64'
          }
          stages {
            stage('Build') {
              steps {
                runningOn(env.NODE_NAME)
                checkout scm

                script {
                  withTools(params.TOOLS_VERSION) {
                      hostAppList = ['usb/host', 'xscope/host']
                      hostAppList.each { app ->
                        dir("examples/${app}") {
                          sh "cmake -B build"
                          sh "make -C build"
                        }
                      }
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        } // Mac arm64 host builds

        stage('Win32 host builds') {
          agent {
            label 'sw-bld-win0'
          }
          stages {
             stage('Build') {
              steps {
                runningOn(env.NODE_NAME)
                checkout scm

                script {
                  // Build the USB host example for 32 bit as libusb is 32 bit
                  withVS('vcvars32.bat') {
                    dir("examples/usb/host") {
                      sh "cmake -G Ninja -B build"
                      sh "ninja -C build"
                    }
                  }
                  // Build the XCOPE host example for 64 bit as XTC tools  32 bit
                  withVS('vcvars64.bat') {
                    withTools(params.TOOLS_VERSION) {
                      dir("examples/xscope/host") {
                        sh "cmake -G Ninja -B build"
                        sh "ninja -C build"
                      }
                    }
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        } // Win32 host builds

      } // parallel
    } // Cross-platform Builds & Tests
  } // stages

  post {
    success {
      updateViewfiles()
    }
    cleanup {
      xcoreCleanSandbox()
    }
  }
}
