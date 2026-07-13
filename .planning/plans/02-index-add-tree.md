# Plan 02: index, add, tree 오브젝트

## 목표

SPEC의 R4 (index와 add), R5 (tree 오브젝트), R3의 tree 출력 부분을 구현한다. 스테이징 영역이라는 개념과 git의 두 가지 바이너리 포맷 (index v2, tree)을 손으로 다뤄보는 것이 학습 포인트다.

## 컨텍스트

- 선행: plan 01 완료. `src/object.c`의 오브젝트 읽기/쓰기 함수와 `minigit init`, `hash-object`가 존재
- index v2 포맷 (공식 문서: git 소스의 `Documentation/gitformat-index.txt`):
  - 헤더 12바이트: `"DIRC"` + 버전(4바이트 빅엔디안, 2) + 엔트리 수
  - 엔트리: ctime/mtime (각 8바이트), dev, ino, mode, uid, gid, size (각 4바이트, 전부 빅엔디안), SHA-1 (20바이트), flags (2바이트, 하위 12비트가 경로 길이), 경로 (NUL 종료), 엔트리 전체를 8바이트 배수로 NUL 패딩
  - 파일 끝: 앞 내용 전체의 SHA-1 체크섬 20바이트
  - 엔트리는 경로 바이트 오름차순 정렬
- tree 오브젝트 내용: 엔트리마다 `"<mode> <name>\0" + <20바이트 raw SHA-1>` 연결. mode는 ASCII, 선행 0 없음 (`100644`, `100755`, 디렉토리는 `40000`)
- tree 정렬 함정: 디렉토리는 이름 뒤에 `/`가 붙은 것처럼 비교해서 정렬한다. `foo.txt`와 디렉토리 `foo`가 섞이면 순서가 index 정렬과 달라질 수 있음
- 학습 팁: 진짜 git으로 add 한 뒤 `xxd .git/index | head -40`으로 실제 바이트를 보면서 파서를 맞추면 빠르다. `git ls-files --stage --debug`도 유용

## 작업

1. `src/index.c`: index 읽기 함수와 쓰기 함수. stat 정보는 `stat()` 결과에서 채움 (nanosecond 필드는 0이어도 git이 읽는 데 문제 없음, 단 진짜 git과의 바이트 비교 테스트에서 차이가 나면 테스트를 `git ls-files --stage` 출력 비교로 잡을 것)
2. `minigit add <file>...`: 각 파일에 대해 blob 저장 → index 엔트리 생성/갱신 (경로 중복 시 교체) → 정렬 유지 → index 쓰기. `.git` 내부 경로 거부. 경로는 바이트열로만 다루고 인코딩 변환 금지
3. `src/tree.c`: index 엔트리들을 디렉토리 계층으로 묶어 하위 디렉토리부터 재귀적으로 tree 오브젝트 생성, 루트 tree 해시 반환. plan 03의 commit이 이 함수를 호출한다
4. tree 생성을 단독 검증할 수단: 임시 서브커맨드 `minigit write-tree` (숨김이어도 됨)를 만들어 `git write-tree`와 비교 가능하게
5. `cat-file -p`에 tree pretty-print 추가: `<mode 6자리 0패딩> <type> <sha>\t<name>` 형식 (git 출력과 동일하게. 저장은 `40000`이지만 출력은 `040000`인 점 주의)
6. `tests/02-index-tree.sh`: 진짜 git과 비교 테스트. 유니코드/공백 파일명, 중첩 경로, `foo`/`foo.txt`/`foo/bar` 정렬 픽스처 포함

## must-haves

- index는 읽기와 쓰기 모두 구현 (add가 기존 index를 읽어 병합해야 하므로)
- tree 정렬 규칙 (디렉토리 `/` 가상 접미사)이 픽스처로 검증됨
- mode 100755가 실제 파일 실행 권한에서 유도됨

## 검증

pass/fail 체크리스트 (SPEC R3, R4, R5에서 이 plan 몫):

- [ ] `minigit add a.txt` 후 `git ls-files --stage` 출력이 진짜 git으로 add 했을 때와 동일하다
- [ ] 같은 파일을 수정 후 다시 add 하면 엔트리가 중복 없이 갱신된다
- [ ] `dir/sub/file.txt` 같은 중첩 경로 add가 동작한다
- [ ] `.git` 내부 경로는 add 대상에서 거부된다
- [ ] 유니코드 파일명 (`한글.txt`)과 공백 포함 파일명이 add에서 깨지지 않는다
- [ ] 동일 index 상태에서 minigit이 만든 tree 해시가 `git write-tree` 해시와 동일하다
- [ ] `foo`, `foo.txt`, `foo/bar` 혼재 픽스처에서 tree 정렬이 git과 일치한다
- [ ] tree 내 디렉토리 엔트리 mode가 선행 0 없는 `40000`으로 기록된다
- [ ] 100644와 100755 두 mode가 실제 파일 권한을 반영한다
- [ ] tree에 대해 `minigit cat-file -p` 출력이 `git cat-file -p` 출력과 일치한다

테스트 명령: `bash tests/02-index-tree.sh`
