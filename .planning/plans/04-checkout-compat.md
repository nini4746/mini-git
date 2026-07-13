# Plan 04: checkout과 호환성 검증 스위트

## 목표

SPEC의 R8 (checkout)과 R9 (호환성 검증 스위트)를 구현한다. 저장소에서 워킹트리로의 역방향 (오브젝트 → 파일)을 완성하고, "진짜 git이 내 저장소를 읽는다"는 승리 조건을 자동 테스트로 못박는다.

## 컨텍스트

- 선행: plan 03 완료. 오브젝트 읽기, tree/commit 파싱, refs 처리가 전부 존재. checkout은 새 포맷 없이 기존 조각의 조합이다
- checkout 동작 정의 (SPEC 확정): `minigit checkout <commit-sha>` → 커밋의 tree를 재귀 순회하며 파일 생성/덮어쓰기 (mode 100755면 실행 권한 부여), 필요한 중간 디렉토리 생성, `.git/HEAD`에 해시를 직접 기록 (detached HEAD). stale 파일 삭제는 범위 외, dirty 체크 없음
- 호환성 스위트는 개별 테스트 (01~03)와 별개로 전체 시나리오를 한 번에 도는 최종 관문. 오브젝트 비교는 압축 해제 후 내용 기준 (압축 바이트는 zlib 레벨에 따라 다를 수 있음)

## 작업

1. `minigit checkout <commit-sha>`: 해시 검증 (오브젝트 존재 + commit 타입 확인, 실패 시 워킹트리 건드리지 않고 에러) → tree 재귀 순회 → blob 내용으로 파일 쓰기 + mode 반영 → HEAD 갱신
2. `tests/04-checkout.sh`: 커밋 A → 수정 → 커밋 B → checkout A → 파일 내용 바이트 비교 시나리오. 중첩 디렉토리와 100755 픽스처 포함
3. `tests/run.sh`: 전체 스위트 러너. 01~04 개별 테스트 실행 + 최종 호환성 시나리오:
   - 시나리오 1 (승리 조건): minigit만으로 init → add → commit x3 → 진짜 `git log`가 3커밋을 올바른 순서로 출력하는지 확인
   - 시나리오 2: 같은 저장소에서 `git fsck` 에러 0건 확인
   - 시나리오 3 (바이트 비교): 고정 env로 동일 입력 시나리오를 minigit 저장소와 git 저장소에서 각각 실행, 양쪽 `.git/objects` 아래 오브젝트 경로 집합이 같고 각 오브젝트의 압축 해제 내용이 같은지 비교 (해제 도구는 python3 zlib 한 줄이면 충분)
   - 실패 시 0이 아닌 종료 코드, 마지막에 pass/fail 요약 출력
4. `Makefile`에 `make test` 타깃 추가 (`bash tests/run.sh` 호출)
5. README의 "검증 아이디어"에 맞게 통과 수치를 셀 수 있도록 러너가 테스트 개수와 통과 개수를 출력

## must-haves

- checkout 실패 경로 (없는 해시)에서 워킹트리 무변경 보장
- 스위트가 임시 디렉토리에서 동작하고 프로젝트 디렉토리를 오염시키지 않음
- must-NOT 확인 항목 포함: minigit 바이너리가 내부에서 git을 호출하지 않는지 (소스에 대한 grep 확인이면 충분)

## 검증

pass/fail 체크리스트 (SPEC R8, R9에서 이 plan 몫):

- [ ] 커밋 A → 수정 → 커밋 B → `minigit checkout <A>` 후 파일 내용이 A 시점과 바이트 단위로 동일하다
- [ ] 중첩 디렉토리와 실행 권한 (100755) 파일이 올바르게 복원된다
- [ ] checkout 후 `.git/HEAD` 내용이 해당 커밋 해시다
- [ ] 존재하지 않는 해시에 대해 워킹트리를 건드리지 않고 에러로 끝난다
- [ ] minigit만으로 만든 저장소에서 진짜 `git log`가 커밋 3개를 올바른 순서로 출력한다 (승리 조건)
- [ ] 같은 저장소에서 `git fsck`가 에러 0건이다
- [ ] 동일 입력에 대해 minigit과 진짜 git의 `.git/objects` 전체 오브젝트 집합이 경로와 압축 해제 후 내용 기준으로 동일하다
- [ ] 테스트 스위트가 단일 명령 (`make test` 또는 `bash tests/run.sh`)으로 실행되고 종료 코드로 성공 여부를 알린다

테스트 명령: `make test`
