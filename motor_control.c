/* ==========================================================================
 * motor_control.c
 * Tang lai cau H 3 pha -- Complementary PWM -- Dead-time --
 * Dong bo hoa lay mau ADC (STM32G474RET6, f_CLK = 170 MHz)
 * Bare-metal / CMSIS register-level, khong dung HAL.
 * ========================================================================== */

#include "stm32g4xx.h"

#define TIM1_CLK_FREQ      170000000UL
#define PWM_FREQ           20000UL     // 20 kHz
#define DEAD_TIME_DTG      0xA6U       // = 1.2 us (tinh toan chi tiet trong bao cao, Phan 2, Cau 1)

/* ------------------------------------------------------------------ */
/* 0. GPIO cho 7 chan TIM1 (CH1/CH2/CH3, CH1N/CH2N/CH3N)               */
/* ------------------------------------------------------------------ */
static void TIM1_3Phase_GPIO_Init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

    /* PC0, PC1, PC2 = TIM1_CH1/CH2/CH3, AF2, Push-Pull */
    GPIOC->MODER &= ~((3u << (0 * 2)) | (3u << (1 * 2)) | (3u << (2 * 2)));
    GPIOC->MODER |=  ((2u << (0 * 2)) | (2u << (1 * 2)) | (2u << (2 * 2))); /* Alternate Function */
    GPIOC->AFR[0] |= (2u << (0 * 4)) | (2u << (1 * 4)) | (2u << (2 * 4));  /* AF2 */

    /* PC13 = TIM1_CH1N, AF4 */
    GPIOC->MODER &= ~(3u << (13 * 2));
    GPIOC->MODER |=  (2u << (13 * 2));
    GPIOC->AFR[1] |= (4u << ((13 - 8) * 4)); /* AF4 */

    /* PB0 = TIM1_CH2N, AF6 */
    GPIOB->MODER &= ~(3u << (0 * 2));
    GPIOB->MODER |=  (2u << (0 * 2));
    GPIOB->AFR[0] |= (6u << (0 * 4)); /* AF6 */

    /* PB15 = TIM1_CH3N, AF4 (doi tu PB1 do PB1 dung cho COMP1_INP) */
    GPIOB->MODER &= ~(3u << (15 * 2));
    GPIOB->MODER |=  (2u << (15 * 2));
    GPIOB->AFR[1] |= (4u << ((15 - 8) * 4)); /* AF4 */

    /* Toc do Low/Medium da du cho ung dung 20 kHz; Push-Pull, No pull-up/down
       (OTYPER giu mac dinh = 0 = Push-Pull, OSPEEDR/PUPDR giu mac dinh) */
}

/* ------------------------------------------------------------------ */
/* Khoi 1 -- TIM1_3Phase_PWM_Init (hoan chinh)                          */
/* ------------------------------------------------------------------ */
void TIM1_3Phase_PWM_Init(void)
{
    /* 0. Bo sung: bat clock GPIO + cau hinh AF cho 7 chan */
    TIM1_3Phase_GPIO_Init();

    /* 1. Bat Clock cho TIM1 */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* TODO 1: ARR cho che do Center-aligned (da tinh trong bao cao, Phan 1.2) */
    uint32_t arr_val = (TIM1_CLK_FREQ / (2 * PWM_FREQ)); // = 4250
    TIM1->ARR = arr_val - 1;                             // = 4249
    TIM1->PSC = 0;                                        // Khong chia tan so

    /* Bo sung: chon Center-aligned Mode 1 (bat buoc theo de, chua co trong skeleton) */
    TIM1->CR1 &= ~TIM_CR1_CMS;
    TIM1->CR1 |=  TIM_CR1_CMS_0; // CMS = 01b: Center-aligned Mode 1

    /* TODO 2: PWM Mode 1 cho CH1, CH2, CH3 */
    TIM1->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM1->CCMR1 |= (6 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;
    TIM1->CCMR2 |= (6 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;
    /* Luu y: skeleton goc cua de co 2 cho ghi nham ten field o khoi CCMR:
       (1) dong CH2 ghi "TIM_CCMR2_OC2M_Pos"/"TIM_CCMR2_OC2PE" -- sai, vi
           truong OC2M/OC2PE nam trong thanh ghi CCMR1 (khong phai CCMR2); da
           sua lai thanh TIM_CCMR1_OC2M_Pos/TIM_CCMR1_OC2PE o dong CH2 phia tren.
       (2) dong CH3 ghi "TIM_CCMR3_OC3PE" -- ten field nay khong ton tai
           (CH3 nam trong thanh ghi CCMR2, khong phai CCMR3; CCMR3 tren STM32G4
           chi dung cho CH5/CH6). Da sua lai thanh dung ten TIM_CCMR2_OC3PE o tren. */

    /* Bo sung: bat output CHx/CHxN -- bat buoc de PWM thuc su xuat ra chan,
       skeleton goc thieu buoc nay */
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1NE
                | TIM_CCER_CC2E | TIM_CCER_CC2NE
                | TIM_CCER_CC3E | TIM_CCER_CC3NE;

    /* TODO 3: Nap Dead-time Generator -- DTG = 0xA6 cho dung 1.2 us */
    TIM1->BDTR &= ~TIM_BDTR_DTG;
    TIM1->BDTR |= (DEAD_TIME_DTG << TIM_BDTR_DTG_Pos);

    /* TODO 4: Break Input -- dung COMP1 noi bo lam nguon (khong dung chan
       BKIN vat ly GPIO, dung theo thiet ke da chot trong bao cao, Phan 1.4) */
    TIM1->BDTR |= TIM_BDTR_BKE;      // Enable Break input logic
    TIM1->BDTR |= TIM_BDTR_BKP;      // Break polarity: Active-High
    TIM1->AF1  |= TIM1_AF1_BKCMP1E;  // Chon output COMP1 lam nguon Break
    TIM1->AF1  |= TIM1_AF1_BKCMP1P;  // Polarity Active-High cho nguon COMP1
    /* QUAN TRONG -- khac voi skeleton goc: KHONG set TIM_BDTR_AOE.
       Skeleton de bai co goi y bat AOE (tu dong bat lai PWM sau Break),
       nhung voi ung dung bao ve qua dong, de AOE=0 (Disable) de bat buoc
       phai reset/xac nhan an toan bang phan mem truoc khi cho PWM chay
       lai -- tranh nguy co tu dong chay lai khi loi chua duoc khac phuc. */

    /* TODO 5: TRGO tai Update Event (Underflow) de dong bo Injected ADC */
    TIM1->CR2 &= ~TIM_CR2_MMS;
    TIM1->CR2 |= (0x2 << TIM_CR2_MMS_Pos);

    /* Cho phep Main Output va khoi dong Counter */
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CR1  |= TIM_CR1_CEN;
}

/* ------------------------------------------------------------------ */
/* Khoi 2 -- ADC_Injected_Trigger_Config (hoan chinh)                   */
/* Trinh tu chuan RM0440: ADVREGEN -> cho on dinh -> ADCAL -> ADEN      */
/* truoc khi cau hinh Injected sequence -- skeleton goc chua de cap,    */
/* neu bo qua ADC se khong hoat dong dung tren phan cung that.          */
/* ------------------------------------------------------------------ */
void ADC_Injected_Trigger_Config(void)
{
    /* Bo sung: bat clock ADC12 + GPIOA, cau hinh PA0/PA1/PA2 = Analog */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER |= (3u << (0 * 2)) | (3u << (1 * 2)) | (3u << (2 * 2)); // Analog mode

    /* Bo sung: bat bo dieu ap noi ADC, cho on dinh theo datasheet (~20 us) */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    for (volatile uint32_t i = 0; i < 3400; i++) { __NOP(); } // ~20us @170MHz (uoc luong, du du an toan)

    /* Bo sung: hieu chuan Single-ended truoc khi bat ADC */
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |=  ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { /* cho hieu chuan xong */ }

    /* Bo sung: bat ADC, cho co san sang ADRDY */
    ADC1->ISR |= ADC_ISR_ADRDY; // xoa co cu (ghi 1 de clear)
    ADC1->CR  |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { /* cho ADC san sang */ }

    /* TODO 6: Cau hinh chuoi Injected -- 3 kenh, trigger = TIM1_TRGO, Rising edge */
    ADC1->JSQR = (2u  << ADC_JSQR_JL_Pos)       // JL = (3 kenh - 1) = 2
               | (0x0u << ADC_JSQR_JEXTSEL_Pos) // 0000 = TIM1_TRGO
               | (0x1u << ADC_JSQR_JEXTEN_Pos)  // 01   = Rising edge
               | (1u  << ADC_JSQR_JSQ1_Pos)     // Rank1 = Channel 1 (PA0, Phase A)
               | (2u  << ADC_JSQR_JSQ2_Pos)     // Rank2 = Channel 2 (PA1, Phase B)
               | (3u  << ADC_JSQR_JSQ3_Pos);    // Rank3 = Channel 3 (PA2, Phase C)

    /* Sampling time = 2.5 cycles (SMP=000) cho kenh 1,2,3 -- theo dung cau hinh CubeMX */
    ADC1->SMPR1 &= ~(ADC_SMPR1_SMP1 | ADC_SMPR1_SMP2 | ADC_SMPR1_SMP3);

    /* Cho phep ngat JEOC de doc JDR1/JDR2/JDR3 trong ISR (khong doc trong while(1)) */
    ADC1->IER |= ADC_IER_JEOCIE;
    NVIC_SetPriority(ADC1_2_IRQn, 0);   // Uu tien cao -- xu ly ngay sau moi chu ky PWM
    NVIC_EnableIRQ(ADC1_2_IRQn);

    /* Khong goi ADC_INJ_Start bang phan mem -- ADC se tu khoi dong chuyen doi
       moi khi nhan canh len tu TIM1_TRGO, dung yeu cau "khong trigger bang
       phan mem" cua de bai. */
}

/* ------------------------------------------------------------------ */
/* Bo sung -- phan con thieu so voi yeu cau de bai "bare-metal hoan     */
/* toan": Clock Config 170MHz thuan CMSIS (khong goi HAL SystemClock_Config) */
/* HSI16 -> PLL -> SYSCLK = 170MHz, PLLM=4, PLLN=85, PLLR=2             */
/* ------------------------------------------------------------------ */
void SystemClock_Config_BareMetal(void)
{
    /* 1. Dam bao HSI16 da bat va on dinh (mac dinh bat sau reset) */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    /* 2. Bat clock PWR, chuyen VOS sang Range 1 + Boost mode
          (bat buoc de chay > 150MHz tren STM32G4) */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR1 &= ~PWR_CR1_VOS;
    PWR->CR1 |=  PWR_CR1_VOS_0;              // VOS = 01: Range 1
    while (PWR->SR2 & PWR_SR2_VOSF) { }      // cho dien ap on dinh
    PWR->CR5 &= ~PWR_CR5_R1MODE;             // R1MODE=0: bat che do Boost

    /* 3. Flash Latency -- PHAI tang truoc khi tang xung nhip.
       LUU Y QUAN TRONG: doi chieu lai bang "Number of wait states"
       trong RM0440 (muc Flash, che do Boost VOS Range1) truoc khi nap --
       neu chon sai so wait-state, CPU fetch lenh sai va co the treo/crash. */
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |=  FLASH_ACR_LATENCY_4WS;    // gia tri tham khao cho 170MHz Boost mode
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS) { }

    /* 4. Cau hinh PLL: HSI16 /PLLM=4 = 4MHz -> xPLLN=85 = 340MHz VCO -> /PLLR=2 = 170MHz */
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }      // cho PLL tat han truoc khi sua cau hinh

    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI;              // Nguon PLL = HSI16
    RCC->PLLCFGR |= (4UL - 1) << RCC_PLLCFGR_PLLM_Pos;   // PLLM = 4
    RCC->PLLCFGR |= 85UL << RCC_PLLCFGR_PLLN_Pos;        // PLLN = 85
    RCC->PLLCFGR |= 0UL  << RCC_PLLCFGR_PLLR_Pos;        // PLLR = 2 (ma 00b)
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;                  // Bat ngo ra PLLR (SYSCLK)

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }   // cho PLL khoa (lock)

    /* 5. Prescaler AHB/APB1/APB2 = /1 (giu 170MHz cho toan bo bus) */
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);

    /* 6. Chuyen SYSCLK sang PLL */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { }

    SystemCoreClockUpdate();   // cap nhat bien CMSIS SystemCoreClock
}

/* ------------------------------------------------------------------ */
/* Bo sung -- ISR doc ADC Injected thuan thanh ghi (khong qua HAL)      */
/* Thay the ADC1_2_IRQHandler() do CubeMX sinh (goi HAL_ADC_IRQHandler) */
/* ------------------------------------------------------------------ */
void ADC1_2_IRQHandler(void)
{
    if (ADC1->ISR & ADC_ISR_JEOC) {
        uint16_t iA = (uint16_t)ADC1->JDR1;   // Dong Phase A
        uint16_t iB = (uint16_t)ADC1->JDR2;   // Dong Phase B
        uint16_t iC = (uint16_t)ADC1->JDR3;   // Dong Phase C

        (void)iA; (void)iB; (void)iC; // tranh canh bao unused-variable khi chua noi FOC

        /* TODO: xu ly FOC / kiem tra nguong qua dong phan mem (du phong) tai day */

        ADC1->ISR |= ADC_ISR_JEOC;   // Xoa co (ghi 1 de clear)
    }
}

/* ------------------------------------------------------------------ */
/* main() -- thu tu bat buoc: GPIO/TIM1 truoc, ADC sau                  */
/* ------------------------------------------------------------------ */
int main(void)
{
    SystemClock_Config_BareMetal();   // Thay HAL_Init()/SystemClock_Config() bang ban CMSIS thuan thanh ghi

    TIM1_3Phase_PWM_Init();           // Khoi 1: PWM 3 pha + Dead-time + Break
    ADC_Injected_Trigger_Config();    // Khoi 2: ADC Injected dong bo TRGO

    while (1) {
        /* Vong lap chinh KHONG doc ADC o day -- du lieu dong dien duoc
           xu ly trong ISR ADC1_2_IRQHandler() ngay khi JEOC xay ra */
    }
}
