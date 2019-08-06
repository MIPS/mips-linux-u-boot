pipeline {
  agent {
    label 'slave_main'
  }

  environment {
    CROSS_COMPILE_MIPS = '/projects/mipssw/toolchains/mips-img-elf/2017.10-05/bin/mips-img-elf-'
    CROSS_COMPILE_NANO = '/projects/mipssw/toolchains/nanomips-elf/2018.01-01/bin/nanomips-elf-'
  }

  stages {
    stage('Build') {
      parallel {
        stage('Boston nano32r6el') {
          steps {
            sh """#!/bin/bash
                  exec ./jenkins-build.sh SUB bostonnano32r6el_defconfig ${CROSS_COMPILE_NANO}"""
          }
        }
        stage('Boston 32r2') {
          steps {
            sh """#!/bin/bash
                  exec ./jenkins-build.sh SUB boston32r2_defconfig ${CROSS_COMPILE_MIPS}"""
          }
        }
        stage('Boston 32r2el') {
          steps {
            sh """#!/bin/bash
                  exec ./jenkins-build.sh SUB boston32r2el_defconfig ${CROSS_COMPILE_MIPS}"""
          }
        }
        stage('Boston 64r2') {
          steps {
            sh """#!/bin/bash
                  exec ./jenkins-build.sh SUB boston64r2_defconfig ${CROSS_COMPILE_MIPS}"""
          }
        }
        stage('Boston 64r2el') {
          steps {
            sh """#!/bin/bash
                  exec ./jenkins-build.sh SUB boston64r2el_defconfig ${CROSS_COMPILE_MIPS}"""
          }
        }
      }
    }

    stage('Coalesce') {
      steps {
        sh """#!/bin/csh
              source /mips/tools/mips/etc/cshrc
              module use /home/pburton/modulefiles
              module load srecord/1.64
              srec_cat -output u-boot.mcs -intel \\
                build-bostonnano32r6el_defconfig/u-boot.mcs -intel \\
                build-boston32r2el_defconfig/u-boot.mcs -intel
              ls -l u-boot.mcs
              srec_info u-boot.mcs -intel
              """
      }
    }

    stage('Upload') {
      steps {
        echo 'TODO'
      }
    }
  }

  post {
    always {
      archive 'u-boot.mcs'
      archive 'build-*/build-env'
      archive 'build-*/stdio'
      archive 'build-*/u-boot'
      archive 'build-*/u-boot.mcs'
      archive 'build-*/.config'
    }
  }
}
