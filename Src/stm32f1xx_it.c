/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "stm32f1xx.h"
#include "stm32f1xx_it.h"

/* USER CODE BEGIN 0 */
#include "cNordLib.h"


#define COL_COUNT 3 ///количество столбцов клавиатуры
#define ROW_COUNT 4 ///количество строк клавиатуры

#define COL_0 (1 << 7)
#define COL_1 (1 << 8)
#define COL_2 (1 << 9)

#define ROW_0 (1 << 3)
#define ROW_1 (1 << 4)
#define ROW_2 (1 << 5)
#define ROW_3 (1 << 6)

#define LONG_CLICK_THRESHOLD 25 ///Порог определения длинного нажатия. Число рассчитано с учетом частоты срабатывания прерываний таймера - 100 Гц. 
																///Для четырехстрочной матрицы, количество опросов одной строки матрицы за одну секунду равно 25 
#define SHORT_CLICK_THRESHOLD 10 ///Минимальное число срабатываний для определения короткого нажатия.
#define MAX_DEVIATION 3					///Максимальная разница между нажатиями двух кнопок


unsigned short keyboardState[ROW_COUNT][COL_COUNT]; //Таблица счетчиков срабатываний для каждой кнопки. 
																										//Увеличение счетчика происходит при срабатывании прерывания возницающего при нажатии или удержании кнопки.

char keyboardSymbols[ROW_COUNT][COL_COUNT] = {{'1','2','3'},
																							{'4','5','6'},
																							{'7','8','9'},
																							{'*','0','#'}
																						 };

/*!
			\brief Функция вызываемая при обнаружении отжатия клавиши.
			\details Данная функция служит для проверки состояния счетчика отжатой кнопки. Если значения счетчика находится в пределах (0;SHORT_CLICK_THRESHOLD] 
			будет проведена проверка наличия второго счетчика со значением находящимся в этом же диапазоне. При обнаружении такого счетчика вызывается функция void double_key_pressed(char key1, char key2).
			В ином случае будет вызвана функция single_key_pressed(char key). 
			После выполнения всех проверок счетчики обнуляются.
			\param[in] currentColumn Номер столбца отжатой клавиши.
			\param[in] currentRow 	 Номер строки отжатой клавиши.			
 */
void clear(unsigned char currentRow,unsigned char currentColumn) 
{
	int i,k; ///счетчики для циклов обнаружения второй нажатой клавиши
	
	if ((keyboardState[currentRow][currentColumn] > 0) && (keyboardState[currentRow][currentColumn] < SHORT_CLICK_THRESHOLD)) ///проверка на вхождение в диапазон определяющий значения котороткого нажатия
	{
		for(i = 0; i < ROW_COUNT;i++)
		for(k = 0; k < COL_COUNT;k++)
		if(((keyboardState[i][k] > 0) && (keyboardState[i][k] < SHORT_CLICK_THRESHOLD)) && ((i != currentRow) || (k != currentColumn))) ///проверка на вхождение в диапазон определяющий значения котороткого нажатия для второй клавиши
		{
			double_key_pressed(keyboardSymbols[currentRow][currentColumn],keyboardSymbols[i][k]);
			
			///обнуление счетчиков
			keyboardState[currentRow][currentColumn] = 0;
			keyboardState[i][k] = 0;
			return;
		}
		
		single_key_pressed(keyboardSymbols[currentRow][currentColumn]);
	}
	
	keyboardState[currentRow][currentColumn] = 0; ///обнуление счетчика
}

/*!
			\brief Функция вызываемая при прерывании вызванном кнопкой с координатами (currentRow,currentColumn).
			\details При срабатывании прерывания происходит увеличение счетчика срабатываний. При совпадении значения счетчика со значением константы LONG_CLICK_THRESHOLD выполняется поиск второго счетчика
			с похожим значением. Если найдено значение отклонение которого не превышает значения MAX_DEVIATION вызывается функция double_key_long_pressed(char key, char key).
			В ином случае будет вызвана функция single_key_long_pressed(char key).
			\param[in] currentColumn Номер столбца отжатой клавиши.
			\param[in] currentRow 	 Номер строки отжатой клавиши.			
 */
void check(unsigned char currentRow, unsigned char currentColumn)
{
	int i,k; ///счетчики для циклов обнаружения второй нажатой клавиши
	
	keyboardState[currentRow][currentColumn]++; //увеличение счетчика с номером клавиши для кторой обнаружено срабатывание 
			
	if(keyboardState[currentRow][currentColumn] == LONG_CLICK_THRESHOLD) //проверка на совпадение со значением константы определяющей длину длинного нажатия
	{		
		///поиск второй нажатой клавиши
		for(i = 0; i < ROW_COUNT;i++)
		for(k = 0; k < COL_COUNT;k++)
		if((keyboardState[i][k] >= (LONG_CLICK_THRESHOLD - MAX_DEVIATION)) && ((i != currentRow) || (k != currentColumn))) //проверка на отклонение второго счетчика от значения LONG_CLICK_THRESHOLD, а также на несовпадения координат с координатами первой клавиши.
		{
			double_key_long_pressed(keyboardSymbols[currentRow][currentColumn],keyboardSymbols[i][k]);
			return;
		}

		single_key_long_pressed(keyboardSymbols[currentRow][currentColumn]);
	}
}
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;

/******************************************************************************/
/*            Cortex-M3 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/*!
			\brief Обработчик прервания контактов PC7-PC9. Вызывается по обнаружению падающего фронта.
			\details При вызове обработчика прерываний определяется номер столбца в котором произошло нажатие клавиши. После определения номера столбца определяется номер строки.
			После того как координаты стали известны - выполняется увеличение счетчика срабатываний для этой клавиши и проверка на определение длинного нажатия.	
 */
void EXTI9_5_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI9_5_IRQn 0 */
	
	int currentColumn = 0;					
	int currentRow = 0;
	
	
	if(!(GPIOC->IDR & COL_0)) ///Проверка регистра Input Data блока GPIOC для определения столбца в котором была нажата клавиша. Значения выражений COL_0 - COL_2 заданы в блоке определения значений #define(строка 45)
	{
		currentColumn = 0;	///Номер текущего столбца - 0
		if(!(GPIOB->IDR & ROW_0)) ///Проверка регистра Input Data блока GPIOB для определения строки в которой была нажата клавиша. Значения выражений ROW_0 - ROW_3 заданы в блоке определения значений #define(строка 49)
		{
			currentRow=0; 									 ///Номер текущей строки - 0
			check(currentRow,currentColumn); ///Увеличение счетчика и проверка его значения для определения длинного нажатия.
		}
		
		///Аналогичные действия выполнены и для других строк
		
		if(!(GPIOB->IDR & ROW_1)) 
		{
			currentRow=1;
			check(currentRow,currentColumn);
		}
		if(!(GPIOB->IDR & ROW_2)) 
		{
			currentRow=2;
			check(currentRow,currentColumn);
		}
		if(!(GPIOB->IDR & ROW_3)) 
		{
			currentRow=3;
			check(currentRow,currentColumn);
		}
	}

	///Аналогичные действия выполнены и для других столбцов
	
	if(!(GPIOC->IDR & COL_1))
	{
		currentColumn = 1;
		if(!(GPIOB->IDR & ROW_0))
		{
			currentRow=0;
			check(currentRow,currentColumn);
		}
		if(!(GPIOB->IDR & ROW_1))
		{
			currentRow=1;
			check(currentRow,currentColumn);
		}
		if(!(GPIOB->IDR & ROW_2))
		{
			currentRow=2;
			check(currentRow,currentColumn);
		}
		if(!(GPIOB->IDR & ROW_3))
		{
			currentRow=3;
			check(currentRow,currentColumn);
		}
	}
	
	if(!(GPIOC->IDR & COL_2))
	{
		currentColumn = 2;
		if(!(GPIOB->IDR & ROW_0))
		{
			currentRow=0;
			check(currentRow,currentColumn);
		}
		if(!(GPIOB->IDR & ROW_1))
		{
			currentRow=1;
			check(currentRow,currentColumn);
		}
		if(!(GPIOB->IDR & ROW_2))
		{
			currentRow=2;
			check(currentRow,currentColumn);
		}
		if(!(GPIOB->IDR & ROW_3))
		{
			currentRow=3;
			check(currentRow,currentColumn);
		}
	}
	
	
  /* USER CODE END EXTI9_5_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
  /* USER CODE BEGIN EXTI9_5_IRQn 1 */

  /* USER CODE END EXTI9_5_IRQn 1 */
}

/*!
			\brief Обработчик прервания таймера
			\details При вызове обработчика прерывания выполняется сдвиг нуля, обеспечивая его "бег". При обнаружении ненажатой клавиши происходит проверка на короткое нажатие и обнуление счетчика срабатываний.
 */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */
	GPIOB->ODR <<= 1; ///Сдвиг на 1 бит влево
	GPIOB->ODR |= 1;  ///Установка единицы в младшем бите
	
	if(!(GPIOB->ODR & 0x80)) GPIOB->ODR = ~(0x08); ///Возвращение бегающего нуля в начало при достижении крайнего левого положения
	
	///Поиск отжатой клавиши
	if(!(GPIOB->ODR & ROW_0)) ///Если при поданом на строку нуле не произошло спада напряжения - проверить на короткое нажатие и очистить счетчик
	{
		if(GPIOC->IDR & COL_0) clear(0,0); 
		if(GPIOC->IDR & COL_1) clear(0,1); 
		if(GPIOC->IDR & COL_2) clear(0,2); 
	}
	
	///Аналогично для остальный строк
	
	if(!(GPIOB->ODR & ROW_1))
	{
		if(GPIOC->IDR & COL_0) clear(1,0); 
		if(GPIOC->IDR & COL_1) clear(1,1); 
		if(GPIOC->IDR & COL_2) clear(1,2); 
	}
	
	if(!(GPIOB->ODR & ROW_2))
	{
		if(GPIOC->IDR & COL_0) clear(2,0); 
		if(GPIOC->IDR & COL_1) clear(2,1); 
		if(GPIOC->IDR & COL_2) clear(2,2); 
	}
	
	if(!(GPIOB->ODR & ROW_3))
	{
		if(GPIOC->IDR & COL_0) clear(3,0); 
		if(GPIOC->IDR & COL_1) clear(3,1); 
		if(GPIOC->IDR & COL_2) clear(3,2); 
	}
	
  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
