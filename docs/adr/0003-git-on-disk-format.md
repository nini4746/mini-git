# 0003. git 온디스크 포맷 호환 (loose object, index v2, loose ref)

- 상태: 채택
- 날짜: 2026-07-13

## 맥락
완료 조건은 minigit으로 만든 저장소를 진짜 git이 읽는 것이다. `git log`, `git fsck`가 통과하고 오브젝트가 바이트 단위로 같아야 한다.
그러려면 저장 포맷이 git과 같아야 한다.

## 결정
- 오브젝트는 `<type> <size>\0<content>`를 SHA-1로 주소화하고 zlib으로 압축해 `.git/objects/xx/나머지38자`에 저장한다. 이미 있는 오브젝트는 다시 쓰지 않는다.
- 타입은 blob, tree, commit만 지원한다. packfile은 쓰지 않는다.
- index는 git index v2 (`DIRC`) 바이너리 포맷이다. 8바이트 정렬 패딩, 경로 기준 정렬, 파일 끝 SHA-1 체크섬을 따른다.
- tree 엔트리는 디렉토리 이름 뒤에 가상 `/`를 붙여 정렬한다. 모드는 `100644`, `100755`, `40000`이다.
- refs는 파일 기반 loose ref다. 기본 브랜치는 `refs/heads/main`이다.

## 대안
- 자체 텍스트 index 포맷: `git ls-files --stage`로 검증할 수 없다.
- packfile, delta 압축: 완료 조건을 loose object만으로 달성할 수 있다. 범위가 크게 늘어난다.
- 기본 브랜치 `master`: SPEC 확정 사항이 `main`이다. 테스트도 `git init -b main`과 비교한다.

## 결과
- 장점: 진짜 git으로 결과를 바로 검증할 수 있다. 상호운용이 양방향이다.
- 단점/감수한 점: packed-refs, tag, symlink (`120000`), submodule (`160000`)은 지원하지 않는다.
