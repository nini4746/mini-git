# Plan 03: commit 오브젝트, refs, log

## 목표

SPEC의 R6 (commit과 refs)과 R7 (log)을 구현한다. 커밋들이 parent 포인터로 연결된 해시 기반 DAG (여기서는 단일 parent 체인)라는 것을 직접 만들어 보는 것이 학습 포인트다.

## 컨텍스트

- 선행: plan 02 완료. index → tree 생성 함수 (`src/tree.c`)가 존재
- commit 오브젝트 내용 (텍스트):
  ```
  tree <루트 tree 해시 40자>
  parent <부모 커밋 해시 40자>        <- 첫 커밋은 이 줄 없음
  author <이름> <<이메일>> <epoch> <타임존>
  committer <이름> <<이메일>> <epoch> <타임존>
  <빈 줄>
  <메시지>
  ```
  타임존은 `+0900` 형식. 메시지 끝에 개행 하나
- refs 해석: `.git/HEAD`가 `ref: refs/heads/main`이면 `.git/refs/heads/main` 파일에서 해시를 읽음. 파일이 없으면 아직 커밋 없음 (unborn branch)
- author/committer 정보: `GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GIT_AUTHOR_DATE` (그리고 `GIT_COMMITTER_*`) env 우선, 없으면 하드코딩 fallback. `GIT_AUTHOR_DATE`는 진짜 git이 받는 `<epoch> <tz>` 형식 (예: `1735689600 +0900`)만 지원하면 충분
- 학습 팁: `git cat-file -p HEAD`로 진짜 커밋 오브젝트의 정확한 바이트를 관찰. 바이트 비교 테스트는 env를 고정해 양쪽을 결정적으로 실행

## 작업

1. `src/refs.c`: HEAD 읽기 (ref 간접 참조 또는 detached 해시), 현재 브랜치 ref 파일 읽기/쓰기
2. `minigit commit -m <msg>`: index 읽기 (비어 있으면 에러) → tree 생성 (plan 02 함수) → parent 결정 (현재 HEAD 해시, unborn이면 생략) → commit 오브젝트 조립/저장 → `refs/heads/main` 갱신 → 해시 출력
3. `src/commit.c`에 커밋 파싱 함수: 오브젝트 내용에서 tree/parent/author/메시지 추출. log가 사용
4. `minigit log`: HEAD 해석 → 커밋 읽기 → 출력 → parent 따라 반복. 출력에 해시, author 이름, 날짜, 메시지 포함 (형식은 git과 완전 일치 불요). 커밋 없으면 에러
5. `cat-file -p`가 commit 타입도 그대로 출력하는지 확인 (텍스트라 blob과 동일 처리로 충분)
6. `tests/03-commit-log.sh`: 고정 env로 minigit과 git 각각 3커밋 시나리오 실행, 커밋 오브젝트 바이트 비교와 해시 목록 비교

## must-haves

- 첫 커밋 (parent 없음)과 이후 커밋 (parent 있음)의 분기가 정확
- env 고정 시 진짜 git과 커밋 해시까지 동일 (해시가 같으면 tree, author 형식, 메시지 개행까지 전부 맞았다는 뜻)
- log가 index가 아니라 오브젝트 저장소만으로 동작 (진짜 git이 만든 저장소에서도 `minigit log`가 동작하면 이상적)

## 검증

pass/fail 체크리스트 (SPEC R6, R7에서 이 plan 몫):

- [ ] 고정 env에서 minigit 커밋 오브젝트가 동일 조건의 진짜 git 커밋과 해시 및 바이트 단위로 동일하다
- [ ] 첫 커밋에는 parent 줄이 없고, 두 번째 커밋부터 직전 커밋이 parent로 기록된다
- [ ] 커밋 후 `.git/refs/heads/main` 내용이 새 커밋 해시 + 개행이다
- [ ] 인덱스가 비어 있으면 커밋을 거부한다 (에러 메시지 + 0이 아닌 종료 코드)
- [ ] 타임스탬프가 `<epoch> <타임존 오프셋>` 형식으로 기록된다
- [ ] 커밋 3개 후 `minigit log`의 해시 목록 (순서 포함)이 `git log --format=%H`와 동일하다
- [ ] log 출력에 각 커밋의 author 이름과 메시지가 포함된다
- [ ] 커밋 없는 저장소에서 `minigit log`가 에러 메시지와 0이 아닌 종료 코드를 낸다

테스트 명령: `bash tests/03-commit-log.sh`
