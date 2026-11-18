# 0004. 작성자 정보는 git 표준 환경변수 우선

- 상태: 채택
- 날짜: 2026-07-13

## 맥락
commit 오브젝트에는 author, committer, 타임스탬프가 들어간다. git과 바이트 단위로 비교하려면 이 값이 결정적이어야 한다.

## 결정
- `GIT_AUTHOR_NAME`, `GIT_AUTHOR_EMAIL`, `GIT_AUTHOR_DATE`와 `GIT_COMMITTER_*`를 먼저 읽는다.
- 없으면 기본값 `minigit <minigit@example.com>`과 현재 시각을 쓴다.
- 타임스탬프는 `<epoch> <tz>` 형식으로 기록한다.

## 대안
- 설정 파일 읽기: 테스트에서 고정 env로 git과 결정적으로 비교하기 어렵다.

## 결과
- 장점: 테스트가 고정 env로 git과 같은 commit 해시를 재현한다.
- 단점/감수한 점: 환경변수는 외부 입력이다. 긴 값에 대비해 버퍼를 실제 필드 길이로 잡아야 한다 (2026-07-14 수정).
