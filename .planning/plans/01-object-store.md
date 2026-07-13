# Plan 01: 오브젝트 저장소 기초 (init, SHA-1, hash-object, cat-file)

## 목표

SPEC의 R1 (init), R2 (blob과 hash-object), R3 (cat-file, blob 부분)을 구현한다. content-addressable storage의 핵심 루프 (내용 → 해시 → 압축 저장 → 다시 읽기)를 완성하는 것이 이 plan의 전부다.

## 컨텍스트

- 프로젝트 루트: `/Users/yunho/project/mini-git`. 현재 README.md와 `.planning/`만 존재. 코드 없음
- 이 plan이 첫 plan이므로 빌드 시스템 (Makefile)도 여기서 만든다
- 언어 C, 링크 `-lz`. SHA-1은 직접 구현 (RFC 3174)
- loose object 포맷 핵심: 저장 내용은 `"<type> <size>\0" + <raw content>`이고, 이 전체에 대한 SHA-1이 오브젝트 ID. 저장 경로는 `.git/objects/<해시 앞 2자>/<나머지 38자>`, 내용은 zlib (deflate, `compress()` 함수면 충분) 압축
- 학습 팁: 진짜 git으로 실험하며 관찰하면 빠르다. 예: `echo hi | git hash-object --stdin`, 저장된 오브젝트 파일을 `python3 -c "import zlib,sys; sys.stdout.buffer.write(zlib.decompress(open(sys.argv[1],'rb').read()))" <path>`로 열어보기

## 작업

1. 뼈대: `Makefile` (cc, `-Wall -Wextra`, `-lz`), `src/main.c`에 서브커맨드 디스패치 (`minigit <command> ...`)
2. `src/sha1.c` + `src/sha1.h`: RFC 3174를 읽고 SHA-1 직접 구현. 블록 패딩과 길이 인코딩 (빅엔디안 64비트)이 함정. 테스트 벡터 3개 이상 (`""`, `"abc"`, 55바이트 초과 입력)으로 자가 검증하는 작은 테스트 실행 파일 또는 assert
3. `minigit init`: `.git/objects`, `.git/refs/heads` 디렉토리와 `.git/HEAD` (`ref: refs/heads/main\n`) 생성. `.git`이 이미 있으면 에러
4. `src/object.c`: 오브젝트 쓰기 함수 (헤더 구성 → SHA-1 → 경로 계산 → zlib 압축 → 파일 쓰기, 이미 존재하면 스킵)와 읽기 함수 (zlib 해제 → 헤더 파싱 → 타입/크기/내용 반환). 이후 plan들이 전부 이 두 함수 위에 선다
5. `minigit hash-object [-w] <file>`: 파일 읽기 → blob 해시 출력, `-w`면 저장
6. `minigit cat-file -t|-s|-p <sha>`: 오브젝트 읽어 타입/크기/내용 출력. 이 시점에는 blob만 있으면 됨 (tree pretty-print는 plan 02에서 추가)
7. `tests/01-objects.sh`: 진짜 git과 비교하는 셸 테스트. 임시 디렉토리에서 minigit과 git 각각 실행해 출력 비교

## must-haves

- 오브젝트 읽기/쓰기가 별도 모듈로 분리되어 이후 plan이 재사용 가능해야 함
- SHA-1 테스트 벡터 검증이 테스트에 포함
- zlib 압축 결과의 바이트 비교가 아니라 압축 해제 후 내용 비교로 검증 (압축 레벨 차이로 압축 바이트는 다를 수 있음. 단 진짜 git이 읽을 수 있어야 하므로 zlib 헤더가 있는 표준 포맷이어야 함)

## 검증

pass/fail 체크리스트 (SPEC R1, R2, R3에서 이 plan 몫):

- [ ] `minigit init` 후 `git status` 실행 시 "not a git repository" 에러가 나지 않는다
- [ ] `.git/HEAD` 내용이 `ref: refs/heads/main` + 개행이다
- [ ] 이미 `.git`이 존재하면 에러 메시지와 0이 아닌 종료 코드로 끝난다
- [ ] SHA-1 구현이 테스트 벡터를 통과한다 (`"abc"` → `a9993e364706816aba3e25717850c26c9cd0d89d`)
- [ ] 임의 파일에 대해 `minigit hash-object <f>` 출력이 `git hash-object <f>` 출력과 동일하다
- [ ] 빈 파일의 blob 해시가 `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`이다
- [ ] `-w`로 저장한 오브젝트를 zlib 해제한 결과가 진짜 git이 저장한 것과 바이트 단위로 동일하다
- [ ] 동일 내용을 두 번 `-w` 해도 저장소 상태가 동일하다
- [ ] blob에 대해 `cat-file -t`는 `blob`, `-s`는 크기, `-p`는 원본 내용을 출력하고 진짜 git 출력과 일치한다
- [ ] 진짜 git이 만든 blob 오브젝트를 `minigit cat-file`로 읽을 수 있다
- [ ] 존재하지 않는 해시에 대해 에러 메시지와 0이 아닌 종료 코드를 낸다

테스트 명령: `bash tests/01-objects.sh`
