# 미니 Git

> 토이 3번. 예상 1~2주. 언어: C 또는 Go - zlib/SHA1만 있으면 됨.

## CS 지식

Content-addressable storage, 해시 기반 자료구조, DAG.

## 왜

- 매일 쓰는 도구 내부. 42/기존 프로젝트에 없는 영역
- content-addressable 개념이 이후 어디서든 재등장 (S3, 캐시, 중복 제거, blockchain도 사실 이것)
- 작음: 주말 2개 규모

## 구성

- 오브젝트 모델: blob / tree / commit, SHA-1 주소화, zlib 압축, `.git/objects` 호환 포맷
- 명령: init, hash-object, cat-file, add(index/스테이징), commit, log, checkout
- diff: Myers 알고리즘 맛보기
- 완료 조건: 진짜 git이 내 미니 git 저장소를 읽을 수 있으면 승리 (`git log` 동작)

## 검증 아이디어

ygo 레포(Yjs 포트, 크로스 픽스처 158개)에서 배운 패턴 - 진짜 git과 오브젝트 바이트 단위 비교 테스트. "호환성 검증 수치"가 곧 README 어필.

## 순서 논리

토이 3~5(Git → DNS → Redis)는 프로토콜/영속화 계열 연쇄. 바이너리 포맷 파싱 감각이 이어짐.
