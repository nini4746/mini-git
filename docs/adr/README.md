# 설계 결정 기록 (ADR)

| 번호 | 결정 | 상태 | 날짜 |
| --- | --- | --- | --- |
| [0001](0001-c-and-zlib-only.md) | C로 구현하고 외부 의존성은 zlib만 사용 | 채택 | 2026-07-13 |
| [0002](0002-sha1-from-scratch.md) | SHA-1 직접 구현 (RFC 3174) | 채택 | 2026-07-13 |
| [0003](0003-git-on-disk-format.md) | git 온디스크 포맷 호환 (loose object, index v2, loose ref) | 채택 | 2026-07-13 |
| [0004](0004-author-from-git-env.md) | 작성자 정보는 git 표준 환경변수 우선 | 채택 | 2026-07-13 |
| [0005](0005-checkout-detached-restore-only.md) | checkout은 detached HEAD와 복원/덮어쓰기만 | 채택 | 2026-07-13 |
| [0006](0006-shell-tests-against-real-git.md) | 테스트는 셸 스크립트로 진짜 git과 비교 | 채택 | 2026-07-13 |
| [0007](0007-myers-basic-diff.md) | Myers diff 기본형 (trace 저장 후 역추적) | 채택 | 2026-07-13 |
| [0008](0008-reject-unsafe-tree-entry-names.md) | checkout에서 안전하지 않은 tree 엔트리 이름 거부 | 채택 | 2026-07-14 |
