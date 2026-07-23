# 미니 Git

> 토이 3번. 언어: C (zlib + 직접 구현한 SHA-1). 진짜 git이 내 저장소를 읽는다.

## CS 지식

Content-addressable storage, 해시 기반 자료구조, DAG.

## 왜

- 매일 쓰는 도구 내부. 42/기존 프로젝트에 없는 영역
- content-addressable 개념이 이후 어디서든 재등장 (S3, 캐시, 중복 제거, blockchain도 사실 이것)
- 완료 조건 달성: minigit으로만 만든 저장소를 진짜 git이 읽음 (`git log`, `git fsck` 통과, 오브젝트 바이트 단위 동일)

## 명령

| 명령 | 설명 |
| --- | --- |
| `init` | `.git/` 생성 (objects, refs/heads, HEAD) |
| `hash-object [-w] <file>` | blob 해시 계산, `-w`면 오브젝트 저장 |
| `cat-file -t\|-s\|-p <sha>` | 오브젝트 타입/크기/내용 출력 |
| `add <file>...` | blob 저장 + 인덱스(스테이징)에 등록 |
| `commit -m <msg>` | 인덱스로 tree 빌드 후 commit 오브젝트 생성, ref 갱신 |
| `log` | HEAD부터 parent 체인 따라 커밋 출력 |
| `checkout <commit-sha>` | 해당 커밋의 tree로 워킹 트리 복원, detached HEAD |
| `diff <fileA> <fileB>` | Myers 알고리즘 기반 라인 diff |
| `write-tree` | 인덱스에서 tree 오브젝트 생성 (테스트용 헬퍼) |

작성자 정보는 `GIT_AUTHOR_NAME` / `GIT_AUTHOR_EMAIL` 등 git 표준 환경변수를 따른다 (미설정 시 기본값).

## 실행

```bash
make          # cc -Wall -Wextra -std=c11, zlib 링크
```

```bash
mkdir demo && cd demo
minigit init                     # Initialized empty minigit repository in .git/
echo 'hello minigit' > hello.txt
minigit add hello.txt
minigit commit -m "first commit" # c76cb5aea89eddea70011afcc618c2ff277bafac
echo 'hello again' >> hello.txt
minigit add hello.txt
minigit commit -m "second commit"
minigit log                      # 두 커밋, parent 체인 역순 출력
minigit checkout c76cb5ae...     # HEAD is now at c76cb5ae... (파일 내용 복원)
git log --oneline                # 진짜 git이 그대로 읽음
```

## 테스트

```bash
make test     # 5개 스위트 + 승리 조건 시나리오, 총 49 checks
```

`tests/run.sh`가 오브젝트 저장소, 인덱스/tree, 커밋/log, checkout, diff 스위트를 돌린 뒤 승리 조건을 검증한다 - minigit으로만 만든 저장소를 진짜 git이 읽는지(`git log` 순서 일치, `git fsck` 무결성), 같은 입력에서 git과 오브젝트 경로/내용이 바이트 단위로 동일한지, 내부에서 git을 호출하지 않는지. 비교용으로 `git`과 `python3` 필요.

## 내부 설계

- 오브젝트 저장소 (`object.c`) - `<type> <size>\0<content>` 형식을 SHA-1로 주소화, zlib 압축해 `.git/objects/xx/나머지38자`에 저장. git과 동일 포맷이라 상호 읽기 가능. 이미 있는 오브젝트는 다시 쓰지 않음 (멱등)
- 인덱스 (`index.c`) - git index v2 (`DIRC`) 바이너리 포맷. 엔트리는 stat 메타데이터 + SHA + 경로, 8바이트 정렬 패딩, 경로 기준 정렬 유지, 파일 끝 SHA-1 체크섬
- tree 빌드 (`tree.c`) - 인덱스에서 디렉토리 단위로 재귀 구성. git의 tree 정렬 규칙(디렉토리 이름 뒤에 가상 `/`를 붙여 비교) 구현, 모드는 100644 / 100755 / 40000
- 커밋 (`commit.c`) - `tree` / `parent` / `author` / `committer` 헤더 + 메시지. 타임스탬프는 `<epoch> <tz>` 형식
- refs (`refs.c`) - `.git/HEAD`는 심볼릭 ref(`ref: refs/heads/main`) 또는 detached 해시. 커밋 시 브랜치 tip(`.git/refs/heads/main`) 갱신, checkout 시 HEAD에 해시 직접 기록
- diff (`diff.c`) - 라인 단위 Myers 최단 편집 스크립트
- SHA-1 (`sha1.c`) - RFC 3174 직접 구현, `minigit sha1-test`로 테스트 벡터 자가 검증
