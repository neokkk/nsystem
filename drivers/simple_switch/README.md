# simple_switch

## 상태 머신을 사용한 LED 제어 스위치 구현

### LED 구성

- output 포트 8개
  - 왼쪽 그룹 (GPIO 17, 27, 22, 5)
  - 오른쪽 그룹 (GPIO 6, 26, 23, 24)

- input 포트 1개
  - GPIO 16

  위에서 아래로, 왼쪽에서 오른쪽으로 인덱스를 매기며, 비트 연산에서는 반대로(0번 인덱스가 LSB) 동작한다.

### 상태
이전 상태와 현재 입력에 따라 다음 상태가 정해진다.
입력은 **OFF**(0), **ON**(1, 1000ms 이내 입력), **TIMEOUT**(1, 1000ms 이후 입력)으로 정의한다.

* **INIT**
  입력이 주어지지 않은(OFF) 기본 상태이다.

* **LIT**
  입력이 발생해 상태가 처음 변화되는 시점이다.
  
  - OFF
    기준 시간 이내 입력이 종료되어 SHORT 상태로 전환된다.
  
  - ON
    기준 시간 이내 입력 중이므로 현재 상태를 유지한다.
  
  - TIMEOUT
    기준 시간(1000ms) 이후까지 입력이 지속되어 LONG 상태로 전환된다.

* **SHORT**
  짧은 입력를 의미한다.
  기준 시간 이내 입력이 발생하면 이후 상태(SHORT SHORT, SHORT LONG)로의 진입을 나타내므로, 바로 액션을 수행하지 않고 대기한다.
  다른 상태로의 전환을 위한 중간 상태이므로, 타임아웃을 감지하는 시간을 500ms로 줄인다.

  - OFF
    추가 입력이 발생할 때까지 현재 상태를 유지하며 대기한다.

  - ON
    기준 시간 이내 추가 입력이 발생하여 SHORT SHORT 상태로 전환된다.

  - TIMEOUT
    기준 시간 이내 추가 입력이 발생하지 않았으므로 SHORT 액션을 수행하고 INIT 상태로 전환된다.

* **LONG**
  긴 입력을 의미한다.
  기준 시간 이내 입력이 발생하면 이후 상태(LONG SHORT, LONG LONG)로의 진입을 나타내므로, 바로 액션을 수행하지 않고 대기한다.
  다른 상태로의 전환을 위한 중간 상태이므로, 타임아웃을 감지하는 시간을 500ms로 줄인다.

  - OFF
    추가 입력이 발생할 때까지 현재 상태를 유지하며 대기한다.

  - ON
    기준 시간 이내 추가 입력이 발생하여 LONG SHORT 상태로 전환된다.

  - TIMEOUT
    기준 시간 이내 추가 입력이 발생하지 않았으므로 LONG 액션을 수행하고 INIT 상태로 전환된다.

* **SHORT SHORT**
  연속된 짧은 입력을 의미한다.

  - OFF
    기준 시간 이내 입력이 종료되어 SHORT SHORT 액션을 수행하고 INIT 상태로 전환된다.

  - ON
    기준 시간 이내 입력 중이므로 현재 상태를 유지한다.

  - TIMEOUT
    기준 시간 이후까지 입력이 지속되어 SHORT LONG 상태로 전환된다.

* **SHORT LONG**
  짧은 입력 이후의 긴 입력을 의미한다.

  - OFF
    기준 시간 이내 입력이 종료되어 SHORT LONG 액션을 수행하고 INIT 상태로 전환된다.

  - ON
    입력 중이므로 현재 상태를 유지한다.

  - TIMEOUT
    입력 중이므로 현재 상태를 유지한다.

* **LONG SHORT**
  긴 입력 이후의 짧은 입력을 의미한다.

  - OFF
    기준 시간 이내 입력이 종료되어 LONG SHORT 액션을 수행하고 INIT 상태로 전환된다.

  - ON
    기준 시간 이내 입력 중이므로 현재 상태를 유지한다.

  - TIMEOUT
    기준 시간 이후까지 입력이 지속되어 LONG LONG 상태로 전환된다.

* **LONG LONG**
  연속된 긴 입력을 의미한다.

  - OFF
    기준 시간 이내 입력이 종료되어 LONG LONG 액션을 수행하고 INIT 상태로 전환된다.

  - ON
    입력 중이므로 현재 상태를 유지한다.

  - TIMEOUT
    입력 중이므로 현재 상태를 유지한다.
  
### 상태 전이도

```mermaid

flowchart TD
	off1["OFF
	(INIT)"] --> on1["ON
	(LIT)"]
	on1 --> off2["OFF
	(SHORT)"]
	on1 --> to1["TIMEOUT
	(LONG)"]
	off2 --> to2["TIMEOUT
	(INIT)"]
	off2 --> on2["ON
	(SHORT SHORT)"]
	to1 --> on3["ON
	(LONG SHORT)"]
	to1 --> off3["OFF
	(INIT)"]
	on2 --> off4["OFF
	(INIT)"]
	on2 --> to3["TIMEOUT
	(SHORT LONG)"]
	on3 --> to4["TIMEOUT
	(LONG LONG)"]
	on3 --> off5["OFF
	(INIT)"]
  ```

### 액션

- AT_NOP
  아무 일도 수행하지 않는다. 기존 상태를 유지하기 위해 사용된다.

- AT_INIT
  입력 시작 시점을 계산하기 위해 현재 시간을 저장한다.

- AT_LED_ON_STRAIGHT
  왼쪽 LED부터 차례로 불이 켜진다.
  SHORT 상태의 액션이다.

- AT_LED_ON_LEFT_4
  왼쪽 LED 4개에 불이 켜진다.
  LONG 상태의 액션이다.

- AT_LED_ON_REVERSE
  현재 켜진 LED의 오른쪽부터 차례로 불이 꺼진다.
  SHORT SHORT 상태의 액션이다.

- AT_LED_ON_LEFT_6
  왼쪽 LED 6개에 불이 켜진다.
  SHORT LONG 상태의 액션이다.

- AT_LED_ON_ALL
  모든 LED의 불의 켜진다.
  LONG SHORT 상태의 액션이다.

- AT_LED_OFF_ALL
  모든 LED의 불이 꺼진다.
  LONG LONG 상태의 액션이다.