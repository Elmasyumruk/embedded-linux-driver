pipeline {
    agent any

    options {
        timestamps()
        buildDiscarder(logRotator(numToKeepStr: '20'))
    }

    stages {
        stage('Environment') {
            steps {
                sh '''
                    echo "kernel  : $(uname -r)"
                    echo "gcc     : $(gcc --version | head -1)"
                    echo "headers : $(ls -d /lib/modules/$(uname -r)/build)"
                    test -d /lib/modules/$(uname -r)/build
                '''
            }
        }

        stage('Build hello module') {
            steps {
                dir('01-hello-module') {
                    sh 'make clean && make'
                }
            }
        }

        stage('Build sigbuf driver') {
            steps {
                dir('02-char-driver') {
                    sh 'make clean && make modules'
                    sh 'modinfo ./sigbuf.ko'
                }
            }
        }

        stage('Build test suite') {
            steps {
                dir('02-char-driver') {
                    sh 'make sigbuf_test'
                    sh 'file ./sigbuf_test'
                }
            }
        }
    }

    post {
        success {
            archiveArtifacts artifacts: '**/*.ko, 02-char-driver/sigbuf_test',
                             fingerprint: true
        }
    }
}
