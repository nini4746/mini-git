# 변경 이력

이 프로젝트의 주요 변경 사항을 기록합니다. 형식은 [Keep a Changelog](https://keepachangelog.com/ko/1.1.0/)를 따릅니다.

## 2026-11-18
### 문서
- README를 아키텍처와 흐름 다이어그램 중심으로 다시 작성
- 요구사항 명세 (SRS)와 설계 명세 (SDS) 추가

## 2026-07-23
### 문서
- README에 명령, 사용법, 테스트, 내부 구조 설명 정리

## 2026-07-14
### 보안
- 긴 `GIT_AUTHOR_NAME` 등 작성자 환경변수로 commit 생성 시 힙 오버플로가 나던 문제 수정
- checkout이 `../evil`, 절대 경로, `.git` 같은 tree 엔트리 이름으로 저장소 밖이나 `.git`에 쓰던 문제 수정

## 2026-07-13
### 추가
- `init`, `hash-object`, `cat-file` 명령과 loose object 저장소 (SHA-1 직접 구현, zlib 압축)
- git index v2 호환 `add`와 index에서 tree를 만드는 `write-tree`
- `commit`과 `log`, `refs/heads/main` 갱신
- `checkout <commit-sha>`로 작업 파일 복원 (detached HEAD)
- Myers 알고리즘 기반 라인 `diff`
- 진짜 git과 결과를 비교하는 셸 테스트 스위트 (`make test`)
