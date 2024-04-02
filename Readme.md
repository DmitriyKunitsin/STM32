#STM32
# Folder with project STM32F407VGT6
## Проект Display №1
![alt text](/Screenshots/image.png)
В данном проекте реализована библиотека для работы с данными дисплеями, которая позволяет обращаться к разным I2C, в связи с тем, что дисплеи одинаковые, то использовал разные I2C
Первый считает до 65000, второй считает каждую 100-ю в первом
> Модель дисплея SSD1306

## Проект PWM №2
![alt text](/Screenshots/image2.png)
В данном проекте реализован таймер с ШИП регулировкой, для плавного включения и отключения светодиодов
> Модель платы STM32F407

## Проект StaticIndication №3
![alt text](/Screenshots/image3.png)
![alt text](/Screenshots/image4.png)
В данном проекте реализован ссветодиодный идикатор для отображения цифр от 0 до 9
> Модель индикатора 5161AS

## Проект Accelerometer №4
![alt text](/Screenshots/image5.png)
В данном проекте реализована работа с акселерометром встроенным в МК, связь идет черещз протоколо SPI, так реализована подсветка нужных светодиодов в зависимости от направления наклона и вывод по UART информации для обработки ее на ПК, приложение для чтения с порта написанного на C# есть в моем GItHub
> Модель платы STM32F407
