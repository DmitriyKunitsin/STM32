## Проект Blink_001
![alt text](./Screenshots/image.png)
Обычное моргание светодиодом, задержка времени реализована через HAL_tick
> Макетная плата STM32F103C8T6
```
while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		//LED_GPIO_Port = GPIOC;
		//LED_Pin = GPIO_PIN_13
		if (flag == 1) {
			if (HAL_GetTick() - T >= 2000) {
				flag = 0;
				T = HAL_GetTick();
			}
		}
		if (flag == 0) {
			if (HAL_GetTick() - T >= 2000) {
				flag = 1;
				T = HAL_GetTick();
			}
		}
		/* OR
		if(flag) {
			GPIOC->BSRR = (uint32_t) LED_Pin << 16u;
		} else {
			GPIOC->BSRR = LED_Pin;
		}
		*/
		HAL_GPIO_WritePin(GPIOC,  LED_Pin, flag);
	}
```
