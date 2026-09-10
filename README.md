# Desktop Cat

Windows 데스크톱의 작업 영역과 다른 창 위에서 움직이는 고양이입니다. 걷기, 대기, 드래그와 던지기, 낙하와 착지, 커서 관찰과 추적을 지원합니다. 커서에 가까워지면 멈춰 바라보다가 대기 상태로 돌아갑니다.

## 코드 구조

| 파일 | 역할 |
| --- | --- |
| `main.cpp` | COM 수명 관리와 프로그램 시작 |
| `Application.*` | 창과 커서 자원, 메시지 처리, 실행 루프 |
| `Cat.*` | 고양이 상태 데이터와 상태 전환 |
| `CatConfig.h` | 크기, 애니메이션, 물리 및 행동 상수 |
| `CatSimulation.*` | 행동, 커서 추적, 드래그, 중력 및 충격 계산 |
| `DesktopPlatforms.*` | 다른 창 검색, 착지, 지지 판정, 창 이동과 가림 처리 |
| `CatSprites.*` | 스프라이트 보관, 방향별 프레임 선택, 애니메이션 |
| `Image.*` | PNG 디코딩, 프레임 추출과 확대/반전, 회전 |
| `PetRenderer.*` | GDI 자원과 투명 창 렌더링 |
| `assets/` | 원본 PNG 5개와 커서 파일 2개 |
| `tests/` | 이미지, 애니메이션, 자원 수명 및 행동 회귀 검사 |

고양이 상태와 자원은 `Application` 인스턴스가 소유합니다. 이미지 로딩은 모든 파일을 검증한 뒤 적용하며, 렌더러와 COM 자원은 실패하거나 종료할 때도 해제됩니다.

## 빌드

Visual Studio의 C++ 데스크톱 개발 도구, v145 툴셋, Windows SDK, C++20을 사용합니다. `Desktop Cat.slnx`를 열거나 Visual Studio Developer PowerShell에서 실행합니다.

```powershell
msbuild '.\Desktop Cat.vcxproj' /p:Configuration=Debug /p:Platform=x64
msbuild '.\Desktop Cat.vcxproj' /p:Configuration=Release /p:Platform=x64
```

32비트 빌드는 `/p:Platform=Win32`를 사용합니다. 필요한 이미지와 커서 파일은 `assets/`에서 각 실행 파일 폴더로 자동 복사됩니다. 실행 파일을 옮길 때는 PNG 5개와 CUR 2개도 함께 옮겨야 합니다.

## 회귀 검사

프로젝트 루트의 Developer PowerShell에서 실행합니다. 테스트용 이미지 복사본은 `tests/fixtures`에 생성됩니다.

```powershell
msbuild '.\tests\Tests.vcxproj' /p:Configuration=Debug /p:Platform=x64
& '.\tests\bin\x64\Tests.exe' '.\assets' '.\tests\fixtures'
```

검사는 프레임 픽셀과 반전, 잘못된 이미지/누락된 파일, 애니메이션 전환, 렌더러 재초기화와 GDI 자원 해제, 드래그/던지기/취소, 착지, 커서 추적 종료를 다룹니다. 실제 데스크톱의 다양한 창 배치 및 모니터 구성에서의 장시간 동작은 수동 확인이 필요합니다.
