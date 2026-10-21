pipeline {
  agent none

  options {
    disableConcurrentBuilds()
    buildDiscarder(logRotator(numToKeepStr: '30'))
  }

  environment {
    REGISTRY = 'registry.internal:5000'
    IMAGE_PREFIX = 'registry.internal:5000/master-db'
  }

  stages {
    stage('betterleaks') {
      agent { label 'betterleaks' }
      steps {
        withChecks('betterleaks') {
          // `dir` scans the checkout. `git` would also scan history, where a
          // deleted test fixture (k8s/test/secrets/regcred.yaml) still trips
          // the generic-api-key rule.
          sh 'betterleaks dir . --redact --no-banner --verbose'
        }
      }
    }

    stage('test (dev)') {
      when {
        not {
          anyOf {
            branch 'main'
            changeRequest target: 'main'
          }
        }
      }
      agent { label 'kind' }
      steps {
        withChecks('test') {
          sh 'git submodule update --init --recursive'
          sh './test.sh'
        }
      }
    }

    stage('test (main)') {
      when {
        anyOf {
          branch 'main'
          changeRequest target: 'main'
        }
      }
      agent { label 'dind' }
      steps {
        withChecks('test') {
          sh 'git submodule update --init --recursive'
          sh './test.sh'
        }
      }
    }

    stage('build-push') {
      when { branch 'dev' }
      agent { label 'dind' }
      steps {
        withChecks('build-push') {
          withCredentials([
            usernamePassword(
              credentialsId: 'registry-creds',
              usernameVariable: 'REGISTRY_USER',
              passwordVariable: 'REGISTRY_PASSWORD'
            )
          ]) {
            sh '''
              set -euo pipefail

              git submodule update --init --recursive
              SHA="$(git rev-parse HEAD)"

              printf '%s' "$REGISTRY_PASSWORD" |
                docker login "$REGISTRY" -u "$REGISTRY_USER" --password-stdin

              docker build \
                --target prod \
                --build-arg USER_ID=25231 \
                -t "$IMAGE_PREFIX/sql-receptionist:dev" \
                -t "$IMAGE_PREFIX/sql-receptionist:$SHA" \
                -f apps/sql-receptionist/Dockerfile .

              docker build \
                --build-arg USER_ID=25231 \
                -t "$IMAGE_PREFIX/db-migrate:dev" \
                -t "$IMAGE_PREFIX/db-migrate:$SHA" \
                -f apps/create_tables/Dockerfile .

              docker push "$IMAGE_PREFIX/sql-receptionist:dev"
              docker push "$IMAGE_PREFIX/sql-receptionist:$SHA"
              docker push "$IMAGE_PREFIX/db-migrate:dev"
              docker push "$IMAGE_PREFIX/db-migrate:$SHA"
            '''
          }
        }
      }
    }
  }
}
