# Motor Control – Tang lai cau H 3 pha (STM32G474RET6)

Bai kiem tra nang luc lap trinh nhung – Cau hinh TIM1/TIM8 (Complementary PWM,
Dead-time, Break) va Injected ADC dong bo hoa lay mau dong dien 3 pha, viet
bare-metal (CMSIS register-level, khong dung HAL).

## Cau hinh phan cung
- MCU: STM32G474RET6 (LQFP64), f_CLK = 170 MHz
- Cong cu: STM32CubeMX (pinout) + build bang Keil MDK-ARM / CMake tuy chon

## Noi dung file
- `motor_control.c` – toan bo code hoan chinh:
  - `SystemClock_Config_BareMetal()` – clock 170 MHz thuan CMSIS
  - `TIM1_3Phase_PWM_Init()` – PWM 3 pha Center-aligned, Dead-time 1.2us, Break qua COMP1
  - `ADC_Injected_Trigger_Config()` – ADC1 Injected 3 kenh, trigger TIM1_TRGO
  - `ADC1_2_IRQHandler()` – doc dong 3 pha tai JEOC
  - `main()`

## Bao cao day du
Xem file `Bai_kiem_tra_Motor_Control_Bao_cao_hoan_chinh.docx` / `.pdf` (Phan 1:
Pinout, Phan 2: Cau hoi giai trinh, Phan 3: Code) di kem trong repo/thu muc goc.

## Ghi chu quan trong
- Da doi chieu toan bo pinout (PA0-PA2, PA6, PB0, PB1, PB15, PC0-PC2, PC13)
  voi datasheet DS12288 Rev 6 (Table 13 – Alternate function) – khop 100%.
- AOE (Automatic Output Enable) duoc tat co chu dich (khac voi skeleton de
  bai) de bat buoc xac nhan an toan bang phan mem truoc khi PWM chay lai
  sau su co Break/qua dong.
