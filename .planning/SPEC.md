# 미니 git 명세

학습용 토이 프로젝트. C로 git의 오브젝트 모델과 핵심 명령을 구현하고, 진짜 git이 결과물을 읽을 수 있게 만든다. 1인 개발, 1~2주 분량. 이 명세와 plan은 학습 가이드 수준이며, 코드는 사용자가 직접 작성한다.

## 목표

`minigit`으로 만든 저장소를 진짜 git이 읽을 수 있으면 승리. 구체적으로 `git log`가 동작하고, `git fsck`가 무오류이며, 생성한 오브젝트가 진짜 git의 오브젝트와 바이트 단위로 동일해야 한다.

## 확정된 결정 사항

- 언어: C (42 배경과 일관성). 빌드는 Makefile, 링크는 `-lz`
- SHA-1: RFC 3174 기반 직접 구현 (학습 목적). zlib은 시스템 라이브러리 사용 (직접 구현 시 범위 폭발)
- index: git index v2 바이너리 포맷 호환 (`git ls-files --stage`로 검증 가능)
- 기본 브랜치: `refs/heads/main`
- author/committer 정보: 환경변수 `GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GIT_AUTHOR_DATE` (committer 계열 포함) 우선, 없으면 하드코딩 fallback. 바이트 비교 테스트는 고정 env로 결정적 실행
- 테스트: 셸 스크립트로 진짜 git과 결과 비교 (내부 구현에서 git 호출 금지, 테스트에서만 git 사용)

## 요구사항

### R1. init

- 검증 문장: `minigit init`이 진짜 git이 유효한 저장소로 인식하는 `.git` 디렉토리를 만든다.
- 현재 상태: 없음 (README.md만 존재)
- 목표 상태: `.git/objects`, `.git/refs/heads`, `.git/HEAD` (내용 `ref: refs/heads/main`)가 생성됨
- 수용 기준:
  - [ ] `minigit init` 후 해당 디렉토리에서 `git status` 실행 시 "not a git repository" 에러가 나지 않는다
  - [ ] `.git/HEAD` 내용이 `ref: refs/heads/main` + 개행이다
  - [ ] 이미 `.git`이 존재하면 에러 메시지를 내고 0이 아닌 종료 코드로 끝난다

### R2. blob 오브젝트와 hash-object

- 검증 문장: `minigit hash-object [-w] <file>`이 진짜 git과 동일한 SHA-1을 출력하고, `-w` 시 진짜 git과 바이트 단위로 동일한 오브젝트 파일을 `.git/objects/xx/yyyy...`에 저장한다.
- 현재 상태: 없음
- 목표 상태: `blob <size>\0<content>` 형식 구성, SHA-1 계산, zlib 압축 저장이 동작
- 수용 기준:
  - [ ] 자체 SHA-1 구현이 테스트 벡터를 통과한다 (`"abc"` → `a9993e364706816aba3e25717850c26c9cd0d89d`)
  - [ ] 임의 파일에 대해 `minigit hash-object <f>` 출력이 `git hash-object <f>` 출력과 동일하다
  - [ ] 빈 파일의 blob 해시가 `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`이다
  - [ ] `-w`로 저장한 오브젝트 파일을 zlib 해제한 결과가 진짜 git이 저장한 것과 바이트 단위로 동일하다
  - [ ] 동일 내용을 두 번 `-w` 해도 결과 저장소 상태가 동일하다 (같은 경로, 같은 내용)

### R3. cat-file

- 검증 문장: `minigit cat-file -t|-s|-p <sha>`가 저장된 오브젝트의 타입, 크기, 내용을 진짜 git과 동일하게 출력한다.
- 현재 상태: 없음
- 목표 상태: 오브젝트 읽기 (zlib 해제 + 헤더 파싱)가 blob/tree/commit 세 타입에 대해 동작
- 수용 기준:
  - [ ] blob에 대해 `-t`는 `blob`, `-s`는 크기, `-p`는 원본 내용을 출력하고 진짜 git 출력과 일치한다
  - [ ] tree에 대해 `-p` 출력이 `git cat-file -p` 출력과 일치한다 (mode, 타입, 해시, 이름)
  - [ ] 진짜 git이 만든 오브젝트도 `minigit cat-file`로 읽을 수 있다 (상호운용 양방향)
  - [ ] 존재하지 않는 해시에 대해 에러 메시지와 0이 아닌 종료 코드를 낸다

### R4. index와 add

- 검증 문장: `minigit add <file>...`이 blob을 저장하고 git index v2 포맷 호환 `.git/index`를 갱신하여, 진짜 git의 `git ls-files --stage`가 읽을 수 있다.
- 현재 상태: 없음
- 목표 상태: index v2 읽기/쓰기 (헤더, 엔트리, 끝의 SHA-1 체크섬)와 add 동작
- 수용 기준:
  - [ ] `minigit add a.txt` 후 `git ls-files --stage` 출력이 진짜 git으로 add 했을 때와 동일하다 (mode, 해시, stage, 경로)
  - [ ] 같은 파일을 수정 후 다시 add 하면 엔트리가 중복 없이 갱신된다
  - [ ] `dir/sub/file.txt` 같은 중첩 경로 add가 동작한다
  - [ ] `.git` 내부 경로는 add 대상에서 거부된다
  - [ ] 파일명은 바이트열로 취급한다: 유니코드 파일명 (예: `한글.txt`)과 공백 포함 파일명이 add 및 이후 commit에서 깨지지 않는다

### R5. tree 오브젝트

- 검증 문장: commit 시 index로부터 만든 tree 오브젝트가 진짜 git의 `git write-tree` 결과와 해시 및 바이트 단위로 동일하다.
- 현재 상태: 없음
- 목표 상태: index 엔트리를 디렉토리 계층으로 묶어 하위 tree부터 재귀 생성
- 수용 기준:
  - [ ] 동일 index 상태에서 minigit이 만든 tree 해시가 `git write-tree` 해시와 동일하다
  - [ ] tree 엔트리 정렬이 git 규칙을 따른다: 디렉토리는 이름 뒤에 `/`가 붙은 것처럼 정렬 (`foo`, `foo.txt`, `foo/bar` 혼재 픽스처로 검증)
  - [ ] tree 내 디렉토리 엔트리 mode가 선행 0 없는 `40000`으로 기록된다
  - [ ] 파일 mode는 `100644` (일반)와 `100755` (실행 가능) 두 가지를 지원하고 실제 파일 권한을 반영한다

### R6. commit 오브젝트와 refs

- 검증 문장: `minigit commit -m <msg>`가 진짜 git과 바이트 단위로 동일한 commit 오브젝트를 만들고 `refs/heads/main`을 갱신한다.
- 현재 상태: 없음
- 목표 상태: `tree`, `parent` (첫 커밋은 생략), `author`, `committer`, 빈 줄, 메시지로 구성된 commit 오브젝트 생성과 ref 파일 갱신
- 수용 기준:
  - [ ] 고정 env (`GIT_AUTHOR_*`, `GIT_COMMITTER_*`)에서 minigit 커밋 오브젝트가 동일 조건의 진짜 git 커밋과 해시 및 바이트 단위로 동일하다
  - [ ] 첫 커밋에는 parent 줄이 없고, 두 번째 커밋부터 직전 커밋이 parent로 기록된다
  - [ ] 커밋 후 `.git/refs/heads/main` 내용이 새 커밋 해시 + 개행이다
  - [ ] 인덱스가 비어 있으면 커밋을 거부한다 (에러 메시지 + 0이 아닌 종료 코드)
  - [ ] 타임스탬프는 `<epoch> <타임존 오프셋>` 형식으로 기록된다

### R7. log

- 검증 문장: `minigit log`가 HEAD에서 시작해 first-parent 체인을 따라 각 커밋의 해시, author, 날짜, 메시지를 출력한다.
- 현재 상태: 없음
- 목표 상태: HEAD → ref → 커밋 해석, parent 따라가기, 커밋 파싱 출력. 출력 형식은 진짜 git과 완전 일치할 필요는 없으나 해시와 커밋 순서는 정확해야 함
- 수용 기준:
  - [ ] 커밋 3개 만든 뒤 `minigit log`에 나온 해시 목록 (순서 포함)이 `git log --format=%H` 출력과 동일하다
  - [ ] 각 커밋의 author 이름과 메시지가 출력에 포함된다
  - [ ] 커밋이 없는 저장소에서는 에러 메시지와 0이 아닌 종료 코드를 낸다

### R8. checkout

- 검증 문장: `minigit checkout <commit-sha>`가 해당 커밋의 tree 내용으로 워킹트리 파일을 복원하고 HEAD를 그 해시로 갱신한다 (detached HEAD).
- 현재 상태: 없음
- 목표 상태: 커밋 → tree 재귀 순회, 파일 생성/덮어쓰기 (mode 반영), 필요한 디렉토리 생성, HEAD를 해시 직접 기록으로 갱신
- 수용 기준:
  - [ ] 커밋 A 생성 → 파일 수정 후 커밋 B → `minigit checkout <A>` 하면 파일 내용이 A 시점과 바이트 단위로 동일하다
  - [ ] 중첩 디렉토리와 실행 권한 (100755) 파일이 올바르게 복원된다
  - [ ] checkout 후 `.git/HEAD` 내용이 해당 커밋 해시다
  - [ ] 존재하지 않는 해시에 대해 워킹트리를 건드리지 않고 에러로 끝난다

### R9. 호환성 검증 스위트

- 검증 문장: 셸 스크립트 테스트 스위트가 진짜 git과의 호환성을 자동으로 pass/fail 판정한다.
- 현재 상태: 없음
- 목표 상태: `tests/` 아래 실행 가능한 스크립트. minigit으로 저장소를 만들고 진짜 git으로 읽는 시나리오 + 오브젝트 바이트 비교
- 수용 기준:
  - [ ] minigit만으로 init → add → commit x3 한 저장소에서 진짜 `git log`가 커밋 3개를 올바른 순서로 출력한다 (승리 조건)
  - [ ] 같은 저장소에서 `git fsck`가 에러 0건이다
  - [ ] 동일 입력 (고정 env)에 대해 minigit과 진짜 git의 `.git/objects` 아래 전체 오브젝트 집합이 경로와 압축 해제 후 내용 기준으로 동일하다
  - [ ] 테스트 스위트가 단일 명령으로 실행되고 (`make test` 또는 `bash tests/run.sh`) 종료 코드로 성공 여부를 알린다

### R10. Myers diff 맛보기 (선택)

- 검증 문장: `minigit diff <fileA> <fileB>`가 Myers 알고리즘으로 두 파일의 라인 단위 최단 편집 스크립트를 계산해 `-`/`+` 형식으로 출력한다.
- 현재 상태: 없음
- 목표 상태: O(ND) Myers 기본형 구현. unified diff 헤더나 hunk 병합은 불요
- 수용 기준:
  - [ ] 동일한 두 파일에 대해 출력이 비어 있다
  - [ ] 알려진 픽스처 (한 줄 삭제 + 한 줄 추가)에 대해 편집 스크립트 라인 수가 `git diff --no-index` 기준 최소 편집 수와 일치한다
  - [ ] 한쪽이 빈 파일이면 전체 추가 또는 전체 삭제로 출력된다

## 경계

### 범위 내

- 명령: init, hash-object, cat-file, add, commit, log, checkout
- 오브젝트: blob, tree, commit (loose object, zlib, SHA-1)
- git index v2 포맷 호환 읽기/쓰기
- refs: `.git/HEAD`, `.git/refs/heads/main` (파일 기반 loose ref)
- 진짜 git과의 상호운용 테스트 스위트
- 선택: Myers diff 맛보기 (파일 대 파일)

### 범위 외 (이유 포함)

- packfile, delta 압축: loose object만으로 승리 조건 달성 가능, 범위 폭발
- packed-refs, tag, annotated tag: 승리 조건에 불필요
- branch/merge/rebase, merge commit (parent 2개 이상): DAG 학습은 log 체인으로 충분, 병합은 별도 프로젝트 규모
- remote, clone, push/pull, 네트워크 전부: 로컬 저장소 학습이 목적
- .gitignore, git status: index 학습은 add로 충분
- symlink (120000), submodule/gitlink (160000): 모드 100644/100755로 학습 목적 달성
- 개행 문자가 포함된 파일명: git quoting 규칙 복잡성 대비 학습 가치 낮음 (엣지 프로브에서 기각)
- 빈 디렉토리 추적: 진짜 git도 추적하지 않음, 동일 동작 (엣지 프로브에서 기각)
- checkout 시 대상 커밋에 없는 stale 파일 삭제: 인덱스 대조 로직이 커져 단순화. 복원과 덮어쓰기만 수행
- SHA-256 오브젝트 포맷: git 기본이 아직 SHA-1, 호환 목표에 SHA-1이면 충분
- 대용량 파일 스트리밍 처리: 학습용, 파일 전체 메모리 로드 허용

## 제약

- 언어 C, 외부 의존성은 zlib (`-lz`)뿐. SHA-1은 직접 구현
- 대상 파일 크기는 수 MB 이하 가정 (전체 메모리 로드)
- 오브젝트, index, refs의 온디스크 포맷은 진짜 git (SHA-1 모드)과 호환
- 테스트는 macOS의 시스템 git으로 검증
- 1인 개발, 1~2주 분량. plan 5개 이하

## must-NOT (부정 수용 기준)

- [ ] minigit 구현 내부에서 `git` 명령을 호출 (shell out) 하지 않는다. git은 테스트 스크립트에서만 사용
- [ ] 특정 입력의 해시 값을 하드코딩해서 테스트를 통과시키지 않는다 (테스트 벡터 상수 비교는 예외)
- [ ] checkout의 명시된 동작 외에 사용자 파일을 삭제하거나 변경하지 않는다
- [ ] `.git` 밖의 경로에 오브젝트나 메타데이터를 쓰지 않는다

## 완료 판정

R1~R9의 모든 수용 기준 체크박스가 pass면 완료. R10은 선택이며 미구현이어도 완료로 인정. 최종 확인은 R9의 테스트 스위트 단일 실행으로 한다.

## 모호성 점수 (최종)

| 차원 | 점수 | 최소 기준 | 통과 |
|---|---|---|---|
| 목표 명확성 | 0.90 | 0.75 | 통과 |
| 경계 명확성 | 0.85 | 0.70 | 통과 |
| 제약 명확성 | 0.80 | 0.65 | 통과 |
| 수용 기준 | 0.90 | 0.70 | 통과 |

모호성 = 1.0 - (0.35x0.90 + 0.25x0.85 + 0.20x0.80 + 0.20x0.90) = 0.1325 (게이트 0.20 이하 통과)

참고: 인터뷰와 그릴링은 상위 지시에 따라 자문자답으로 수행됨.
