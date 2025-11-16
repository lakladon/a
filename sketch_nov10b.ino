/*
  БЛЯТЬ ЭТО МОЯ ИГРА НА РЕАКЦИЮ С КРУТЫМ МЕНЮ И СОХРАНЕНИЕМ РЕКОРДОВ!
  ТУТ ЕСТЬ ВСЁ ЧТО НУЖНО ДЛЯ КРУТОЙ ИГРЫ:
  - МЕНЮШКА КАК У ВЗРОСЛЫХ
  - СОХРАНЕНИЕ РЕКОРДОВ В ПАМЯТИ ESP32
  - СТАТИСТИКА ИГР
  - КРУТАЯ ЗАСТАВКА
  - И ЕЩЕ КУЧА ПРИКОЛОВ!

  АВТОР: LAKLADON (ЭТО Я)
  ВЕРСИЯ: 3.0 С МЕНЮ И ИСПРАВЛЕННЫМИ БАГАМИ
*/

#include <Wire.h>          // ЭТА БИБЛИОТЕКА ДЛЯ ОБЩЕНИЯ ПО I2C С ДИСПЛЕЕМ
#include <Adafruit_GFX.h>  // ЭТА ДЛЯ РИСОВАНИЯ ГРАФИКИ И ТЕКСТА НА ЭКРАНЕ
#include <Adafruit_SSD1306.h> // А ЭТА КОНКРЕТНО ДЛЯ НАШЕГО ДИСПЛЕЯ SSD1306
#include <Preferences.h>   // ЭТА ДЛЯ СОХРАНЕНИЯ ДАННЫХ В ПАМЯТИ ESP32 (КАК EEPROM ТОЛЬКО ЛУЧШЕ)

// НАСТРОЙКИ ДИСПЛЕЯ - РАЗМЕРЫ ЭКРАНА В ПИКСЕЛЯХ
#define WIDTH 128    // ШИРИНА ЭКРАНА - 128 ТОЧЕК (МОЖНО РИСОВАТЬ 128 ПИКСЕЛЕЙ ПО ГОРИЗОНТАЛИ)
#define HEIGHT 64    // ВЫСОТА ЭКРАНА - 64 ТОЧКИ (МОЖНО РИСОВАТЬ 64 ПИКСЕЛЯ ПО ВЕРТИКАЛИ)
#define OLED_RST -1  // ПИН СБРОСА ДИСПЛЕЯ (НЕ ИСПОЛЬЗУЕТСЯ, МОЖНО ИГНОРИРОВАТЬ)

// ПИНЫ КНОПОК НА ESP32 - ТУТ Я ПОДКЛЮЧИЛ 4 КНОПКИ
#define BTN1   12  // КНОПКА 1: В МЕНЮ - ВВЕРХ, В ИГРЕ - КНОПКА 1
#define BTN2   14  // КНОПКА 2: В МЕНЮ - ВНИЗ, В ИГРЕ - КНОПКА 2
#define BTN3   27  // КНОПКА 3: В МЕНЮ - ВЫБОР, В ИГРЕ - КНОПКА 3
#define BTN4   26  // КНОПКА 4: В МЕНЮ - НАЗАД, В ИГРЕ - КНОПКА 4

// СОЗДАЕМ ОБЪЕКТ ДИСПЛЕЯ - ЭТО КАК МОНИТОР ДЛЯ НАШЕЙ ИГРЫ
Adafruit_SSD1306 screen(WIDTH, HEIGHT, &Wire, OLED_RST);

// СОЗДАЕМ ОБЪЕКТ ДЛЯ РАБОТЫ С ПАМЯТЬЮ ESP32 - ЧТОБЫ СОХРАНЯТЬ РЕКОРДЫ
Preferences preferences;

// ПЕРЕЧИСЛЕНИЕ СОСТОЯНИЙ ПРОГРАММЫ - ЭТО КАК РАЗНЫЕ ЭКРАНЫ В ИГРЕ
enum ProgramState {
  MAIN_MENU,      // ГЛАВНОЕ МЕНЮ - ТУТ ВЫБИРАЕМ ЧТО ДЕЛАТЬ
  IN_GAME,        // НАЧАЛО ИГРЫ - ЭКРАН "PRESS ANY BUTTON TO START"
  READY_SCREEN,   // ЭКРАН "ГОТОВЬСЯ" - ЖДЕМ СЛУЧАЙНОЕ ВРЕМЯ
  GAME_SCREEN,    // ИГРОВОЙ ЭКРАН - ПОКАЗЫВАЕТ ЦИФРУ КОТОРУЮ НАДО НАЖАТЬ
  WIN_SCREEN,     // ЭКРАН ПОБЕДЫ - КОГДА УСПЕЛ НАЖАТЬ ПРАВИЛЬНУЮ КНОПКУ ВОВРЕМЯ
  GAME_OVER,      // ЭКРАН ПРОИГРЫША - КОГДА ПРОИГРАЛ
  BEST_SCREEN,    // ЭКРАН С РЕКОРДАМИ И СТАТИСТИКОЙ
  SOURCE_SCREEN,  // ЭКРАН С ИНФОЙ ОБ ИСХОДНОМ КОДЕ
  ABOUT_SCREEN    // ЭКРАН "ОБ ИГРЕ"
};

ProgramState currentState = MAIN_MENU;  // ТЕКУЩЕЕ СОСТОЯНИЕ - НАЧИНАЕМ С ГЛАВНОГО МЕНЮ

// ПЕРЕМЕННЫЕ ДЛЯ РАБОТЫ С МЕНЮ
int menuPosition = 0;  // ТЕКУЩАЯ ПОЗИЦИЯ В МЕНЮ (0-3)
const char* menuItems[] = {"NEW GAME", "BEST", "SOURCE", "ABOUT"};  // ПУНКТЫ МЕНЮ

// ПЕРЕМЕННЫЕ ДЛЯ ИГРЫ - ТУТ ХРАНИТСЯ ВСЯ ИГРОВАЯ ИНФОРМАЦИЯ
unsigned long startGameTime;        // ВРЕМЯ КОГДА НАЧАЛСЯ ТЕКУЩИЙ РАУНД (В МИЛЛИСЕКУНДАХ)
unsigned long pressButtonTime;      // ВРЕМЯ КОГДА НАЖАЛИ КНОПКУ (ЧТОБЫ ПОСЧИТАТЬ РЕАКЦИЮ)
unsigned long readyScreenTime;      // ВРЕМЯ КОГДА ПОКАЗАЛИ ЭКРАН "ГОТОВЬСЯ"
unsigned long randomDelayTime;      // СЛУЧАЙНАЯ ЗАДЕРЖКА ПЕРЕД ПОКАЗОМ ЦИФРЫ (ОТ 2 ДО 5 СЕКУНД)
int needButton = 0;                 // КАКУЮ КНОПКУ НУЖНО НАЖАТЬ (0-3 СООТВЕТСТВУЕТ BTN1-BTN4)
int points = 0;                     // ТЕКУЩИЕ ОЧКИ (ЧЕМ БОЛЬШЕ ТЕМ КРУЧЕ)
float bestReaction = 0.0;           // ЛУЧШЕЕ ВРЕМЯ РЕАКЦИИ ЗА ВСЕ ИГРЫ (РЕКОРД)
float nowReaction = 0.0;            // ВРЕМЯ РЕАКЦИИ В ТЕКУЩЕМ РАУНДЕ
float timeForPress = 1.0;           // СКОЛЬКО ВРЕМЕНИ ДАЕТСЯ НА НАЖАТИЕ (УМЕНЬШАЕТСЯ С УРОВНЕМ)
int hardLevel = 1;                  // ТЕКУЩИЙ УРОВЕНЬ СЛОЖНОСТИ (ОТ 1 ДО 11)

// СТАТИСТИКА ИГР - СОБИРАЕМ ДАННЫЕ ОБО ВСЕХ ИГРАХ
int totalGames = 0;    // ОБЩЕЕ КОЛИЧЕСТВО СЫГРАННЫХ ИГР
int totalWins = 0;     // СКОЛЬКО РАЗ ИГРОК ПОБЕДИЛ (УСПЕЛ НАЖАТЬ ВОВРЕМЯ)
int totalPoints = 0;   // СУММА ВСЕХ НАБРАННЫХ ОЧКОВ ЗА ВСЕ ИГРЫ

// ТЕКСТ ДЛЯ ОТОБРАЖЕНИЯ ЦИФР КНОПОК
const char* buttonText[4] = {"1", "2", "3", "4"};

// ФУНКЦИЯ SETUP - ВЫПОЛНЯЕТСЯ ОДИН РАЗ ПРИ ВКЛЮЧЕНИИ ESP32
void setup() {
  Serial.begin(115200);  // ВКЛЮЧАЕМ ПОСЛЕДОВАТЕЛЬНЫЙ ПОРТ ДЛЯ ОТЛАДКИ (ЧТОБЫ ВИДЕТЬ ЧТО ПРОИСХОДИТ)
  
  // НАСТРАИВАЕМ ПИНЫ КНОПОК КАК ВХОДЫ С ПОДТЯЖКОЙ К ПИТАНИЮ
  pinMode(BTN1, INPUT_PULLUP);  // КНОПКА 1 - КОГДА НЕ НАЖАТА = HIGH, НАЖАТА = LOW
  pinMode(BTN2, INPUT_PULLUP);  // КНОПКА 2 - ТАК ЖЕ КАК И ПЕРВАЯ
  pinMode(BTN3, INPUT_PULLUP);  // КНОПКА 3 - ВСЕ КНОПКИ ПОДКЛЮЧЕНЫ ОДИНАКОВО
  pinMode(BTN4, INPUT_PULLUP);  // КНОПКА 4 - ПОСЛЕДНЯЯ КНОПКА
  
  // ПЫТАЕМСЯ ИНИЦИАЛИЗИРОВАТЬ ДИСПЛЕЙ
  if(!screen.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Display error");  // ЕСЛИ НЕ ПОЛУЧИЛОСЬ - ПИШЕМ ОШИБКУ
    for(;;); // ЗАВИСАЕМ В ВЕЧНОМ ЦИКЛЕ - БЕЗ ДИСПЛЕЯ ИГРА НЕ ИМЕЕТ СМЫСЛА
  }
  
  // НАСТРАИВАЕМ ДИСПЛЕЙ
  screen.clearDisplay();    // ОЧИЩАЕМ ЭКРАН ОТ ВСЯКОГО МУСОРА
  screen.setTextColor(WHITE); // УСТАНАВЛИВАЕМ БЕЛЫЙ ЦВЕТ ТЕКСТА (НА ЧЕРНОМ ФОНЕ)
  
  loadAllData();    // ЗАГРУЖАЕМ СОХРАНЕННЫЕ ДАННЫЕ ИЗ ПАМЯТИ
  showCoolStart();  // ПОКАЗЫВАЕМ КРУТУЮ ЗАСТАВКУ
  showMainMenu();   // ПОКАЗЫВАЕМ ГЛАВНОЕ МЕНЮ
}

// ФУНКЦИЯ ДЛЯ ЗАГРУЗКИ ВСЕХ СОХРАНЕННЫХ ДАННЫХ ИЗ ПАМЯТИ ESP32
void loadAllData() {
  preferences.begin("reaction_game", false);  // ОТКРЫВАЕМ ПАМЯТЬ С ИМЕНЕМ "reaction_game"
  
  // ЗАГРУЖАЕМ ВСЕ ЗНАЧЕНИЯ, ЕСЛИ ЧЕГО-ТО НЕТ - ИСПОЛЬЗУЕМ ЗНАЧЕНИЕ ПО УМОЛЧАНИЮ
  bestReaction = preferences.getFloat("best_time", 0.0);      // ЛУЧШЕЕ ВРЕМЯ (ПО УМОЛЧАНИЮ 0.0)
  totalGames = preferences.getInt("total_games", 0);          // ВСЕГО ИГР (ПО УМОЛЧАНИЮ 0)
  totalWins = preferences.getInt("total_wins", 0);            // ПОБЕД (ПО УМОЛЧАНИЮ 0)
  totalPoints = preferences.getInt("total_points", 0);        // ВСЕГО ОЧКОВ (ПО УМОЛЧАНИЮ 0)
  
  preferences.end();  // ЗАКРЫВАЕМ ПАМЯТЬ - ВАЖНО ДЕЛАТЬ ЭТО ПОСЛЕ РАБОТЫ С ПАМЯТЬЮ
  
  // ВЫВОДИМ В СЕРИАЛ ДЛЯ ОТЛАДКИ
  Serial.println("=== ДАННЫЕ ЗАГРУЖЕНЫ ИЗ ПАМЯТИ ESP32 ===");
  Serial.print("Лучшее время: "); Serial.println(bestReaction);
  Serial.print("Всего игр: "); Serial.println(totalGames);
  Serial.print("Побед: "); Serial.println(totalWins);
  Serial.print("Всего очков: "); Serial.println(totalPoints);
}

// ФУНКЦИЯ ДЛЯ СОХРАНЕНИЯ ВСЕХ ДАННЫХ В ПАМЯТЬ ESP32
void saveAllData() {
  preferences.begin("reaction_game", false);  // ОТКРЫВАЕМ ПАМЯТЬ
  
  // СОХРАНЯЕМ ВСЕ ЗНАЧЕНИЯ
  preferences.putFloat("best_time", bestReaction);  // ЛУЧШЕЕ ВРЕМЯ
  preferences.putInt("total_games", totalGames);    // КОЛИЧЕСТВО ИГР
  preferences.putInt("total_wins", totalWins);      // КОЛИЧЕСТВО ПОБЕД
  preferences.putInt("total_points", totalPoints);  // СУММА ОЧКОВ
  
  preferences.end();  // ЗАКРЫВАЕМ ПАМЯТЬ
  
  Serial.println("=== ДАННЫЕ СОХРАНЕНЫ В ПАМЯТЬ ESP32 ===");
}

// КРУТАЯ ЗАСТАВКА С АНИМАЦИЕЙ - ПОКАЗЫВАЕТ МОЙ НИК И ПЛАВНО ИСЧЕЗАЕТ
void showCoolStart() {
  screen.clearDisplay();  // НАЧИНАЕМ С ЧИСТОГО ЭКРАНА
  
  // АНИМАЦИЯ ПОЯВЛЕНИЯ ТЕКСТА "LAKLADON" ПО БУКВАМ
  for(int i = 0; i <= 8; i++) {  // 8 БУКВ В СЛОВЕ "LAKLADON"
    screen.clearDisplay();  // КАЖДЫЙ РАЗ ЧИСТИМ ЭКРАН ДЛЯ ПЛАВНОСТИ
    screen.setTextSize(1);  // УСТАНАВЛИВАЕМ РАЗМЕР ТЕКСТА 1 (МАЛЕНЬКИЙ)
    screen.setCursor(30, 20); // СТАВИМ КУРСОР В ПОЗИЦИЮ (30,20)
    
    String text = "LAKLADON";  // МОЙ КРУТОЙ НИК
    for(int j = 0; j < i; j++) {  // ПЕЧАТАЕМ СТОЛЬКО БУКВ, СКОЛЬКО УЖЕ ДОЛЖНО БЫТЬ ВИДНО
      screen.print(text[j]);  // ПЕЧАТАЕМ ОЧЕРЕДНУЮ БУКВУ
    }
    
    // КОГДА ВЕСЬ ТЕКСТ НАПЕЧАТАН, ДОБАВЛЯЕМ АНИМАЦИЮ ТОЧЕК
    if(i == 8) {
      for(int dot = 0; dot < 4; dot++) {  // 4 ТОЧКИ КОТОРЫЕ ПОЯВЛЯЮТСЯ ПО ОДНОЙ
        screen.clearDisplay();  // ОПЯТЬ ЧИСТИМ ЭКРАН
        screen.setCursor(30, 20); // СТАВИМ КУРСОР ДЛЯ ОСНОВНОГО ТЕКСТА
        screen.print("LAKLADON"); // ПЕЧАТАЕМ ВЕСЬ ТЕКСТ
        screen.setCursor(35, 35); // СТАВИМ КУРСОР ДЛЯ ПОДПИСИ
        screen.setTextSize(1);  // МАЛЕНЬКИЙ ТЕКСТ ДЛЯ ПОДПИСИ
        screen.print("presents"); // ПИШЕМ "PRESENTS" (ТИПА Я ПРЕДСТАВЛЯЮ ИГРУ)
        
        // РИСУЕМ ТОЧКИ КОТОРЫЕ ПОЯВЛЯЮТСЯ ПО ОДНОЙ (ТИПА LOADING...)
        screen.setCursor(55 + dot * 5, 45); // ДВИГАЕМ КУРСОР ДЛЯ КАЖДОЙ НОВОЙ ТОЧКИ
        for(int d = 0; d <= dot; d++) {  // ПЕЧАТАЕМ ВСЕ ТОЧКИ КОТОРЫЕ УЖЕ ДОЛЖНЫ БЫТЬ ВИДНЫ
          screen.print(".");  // ПРОСТО ПЕЧАТАЕМ ТОЧКУ
        }
        screen.display();  // ПОКАЗЫВАЕМ ЧТО ПОЛУЧИЛОСЬ
        delay(300);  // ЖДЕМ 300 МС ЧТОБЫ БЫЛО ВИДНО АНИМАЦИЮ
      }
    } else {
      screen.display();  // ЕСЛИ ЕЩЕ НЕ ВСЕ БУКВЫ - ПОКАЗЫВАЕМ ТЕКУЩЕЕ СОСТОЯНИЕ
      delay(200);  // ЖДЕМ 200 МС МЕЖДУ ПОЯВЛЕНИЕМ БУКВ
    }
  }
  
  delay(1000);  // ЖДЕМ СЕКУНДУ ЧТОБЫ ИГРОК УСПЕЛ УВИДЕТЬ ЗАСТАВКУ
  
  // АНИМАЦИЯ ИСЧЕЗНОВЕНИЯ - ЧЕРНЫЕ ЛИНИИ ПОСТЕПЕННО ЗАПОЛНЯЮТ ЭКРАН
  for(int i = 0; i < 16; i++) {  // 16 ШАГОВ ИСЧЕЗНОВЕНИЯ (16*4=64 - ВСЯ ВЫСОТА ЭКРАНА)
    screen.clearDisplay();  // ЧИСТИМ ЭКРАН
    screen.setTextSize(1);  // РАЗМЕР ТЕКСТА 1
    screen.setCursor(30, 20); // СТАВИМ КУРСОР ДЛЯ ОСНОВНОГО ТЕКСТА
    screen.print("LAKLADON"); // ПЕЧАТАЕМ ТЕКСТ
    screen.setTextSize(1);  // РАЗМЕР ТЕКСТА 1
    screen.setCursor(35, 35); // СТАВИМ КУРСОР ДЛЯ ПОДПИСИ
    screen.print("presents"); // ПЕЧАТАЕМ ПОДПИСЬ
    
    // РИСУЕМ ЧЕРНЫЕ ЛИНИИ КОТОРЫЕ ПОСТЕПЕННО ЗАПОЛНЯЮТ ЭКРАН СВЕРХУ ВНИЗ
    for(int y = 0; y < HEIGHT; y += 4) {  // ИДЕМ ПО ВСЕЙ ВЫСОТЕ ЭКРАНА С ШАГОМ 4 ПИКСЕЛЯ
      if(y >= i * 4) {  // ЕСЛИ ТЕКУЩАЯ СТРОЧКА ДОЛЖНА БЫТЬ ЧЕРНОЙ
        screen.drawFastHLine(0, y, WIDTH, BLACK);  // РИСУЕМ ЧЕРНУЮ ЛИНИЮ ВО ВСЮ ШИРИНУ ЭКРАНА
      }
    }
    screen.display();  // ПОКАЗЫВАЕМ ЧТО ПОЛУЧИЛОСЬ
    delay(50);  // ЖДЕМ 50 МС ДЛЯ БЫСТРОЙ АНИМАЦИИ
  }
  
  screen.clearDisplay();  // ОКОНЧАТЕЛЬНО ЧИСТИМ ЭКРАН
  screen.display();  // И ПОКАЗЫВАЕМ ЧИСТЫЙ ЭКРАН (ГОТОВ К РАБОТЕ)
}

// ГЛАВНЫЙ ЦИКЛ - ВЫПОЛНЯЕТСЯ ПОСТОЯННО ПОКА РАБОТАЕТ ESP32
void loop() {
  // В ЗАВИСИМОСТИ ОТ ТЕКУЩЕГО СОСТОЯНИЯ ВЫЗЫВАЕМ СООТВЕТСТВУЮЩУЮ ФУНКЦИЮ ОБРАБОТКИ
  switch(currentState) {
    case MAIN_MENU:      // ЕСЛИ МЫ В ГЛАВНОМ МЕНЮ
      handleMainMenu();  // ОБРАБАТЫВАЕМ ГЛАВНОЕ МЕНЮ
      break;
    case IN_GAME:        // ЕСЛИ МЫ В НАЧАЛЕ ИГРЫ (ЭКРАН "PRESS ANY BUTTON")
      handleInGame();    // ОБРАБАТЫВАЕМ НАЧАЛО ИГРЫ
      break;
    case READY_SCREEN:   // ЕСЛИ МЫ НА ЭКРАНЕ "ГОТОВЬСЯ"
      handleReadyScreen(); // ОБРАБАТЫВАЕМ ЭКРАН ГОТОВНОСТИ
      break;
    case GAME_SCREEN:    // ЕСЛИ МЫ НА ИГРОВОМ ЭКРАНЕ (ПОКАЗАНА ЦИФРА)
      handleGameScreen(); // ОБРАБАТЫВАЕМ ИГРОВОЙ ЭКРАН
      break;
    case WIN_SCREEN:     // ЕСЛИ МЫ НА ЭКРАНЕ ПОБЕДЫ
      handleWinScreen();  // ОБРАБАТЫВАЕМ ЭКРАН ПОБЕДЫ
      break;
    case GAME_OVER:      // ЕСЛИ МЫ НА ЭКРАНЕ ПРОИГРЫША
      handleGameOver();  // ОБРАБАТЫВАЕМ ЭКРАН ПРОИГРЫША
      break;
    case BEST_SCREEN:    // ЕСЛИ МЫ НА ЭКРАНЕ С РЕКОРДАМИ
    case SOURCE_SCREEN:  // ЕСЛИ МЫ НА ЭКРАНЕ С ИСХОДНЫМ КОДОМ
    case ABOUT_SCREEN:   // ЕСЛИ МЫ НА ЭКРАНЕ "ОБ ИГРЕ"
      handleInfoScreens(); // ОБРАБАТЫВАЕМ ИНФОРМАЦИОННЫЕ ЭКРАНЫ
      break;
  }
}

// ФУНКЦИЯ ДЛЯ ОБРАБОТКИ ГЛАВНОГО МЕНЮ
void handleMainMenu() {
  // ПРОВЕРЯЕМ КНОПКУ ВВЕРХ (BTN1)
  if (digitalRead(BTN1) == LOW) {
    delay(200);  // ЖДЕМ 200 МС ДЛЯ АНТИДРЕБЕЗГА
    menuPosition--;  // ПЕРЕМЕЩАЕМСЯ ВВЕРХ ПО МЕНЮ
    if (menuPosition < 0) menuPosition = 3;  // ЕСЛИ ВЫШЛИ ЗА ВЕРХ - ПЕРЕХОДИМ В НИЗ
    showMainMenu();  // ОБНОВЛЯЕМ ОТОБРАЖЕНИЕ МЕНЮ
  }
  
  // ПРОВЕРЯЕМ КНОПКУ ВНИЗ (BTN2)
  if (digitalRead(BTN2) == LOW) {
    delay(200);  // ЖДЕМ 200 МС ДЛЯ АНТИДРЕБЕЗГА
    menuPosition++;  // ПЕРЕМЕЩАЕМСЯ ВНИЗ ПО МЕНЮ
    if (menuPosition > 3) menuPosition = 0;  // ЕСЛИ ВЫШЛИ ЗА НИЗ - ПЕРЕХОДИМ В ВЕРХ
    showMainMenu();  // ОБНОВЛЯЕМ ОТОБРАЖЕНИЕ МЕНЮ
  }
  
  // ПРОВЕРЯЕМ КНОПКУ ВЫБОРА (BTN3)
  if (digitalRead(BTN3) == LOW) {
    delay(200);  // ЖДЕМ 200 МС ДЛЯ АНТИДРЕБЕЗГА
    selectMenuItem();  // ВЫБИРАЕМ ТЕКУЩИЙ ПУНКТ МЕНЮ
  }
}

// ФУНКЦИЯ ДЛЯ ОТОБРАЖЕНИЯ ГЛАВНОГО МЕНЮ НА ЭКРАНЕ
void showMainMenu() {
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  
  // РИСУЕМ ЗАГОЛОВОК "REACTION" КРУПНЫМ ШРИФТОМ
  screen.setTextSize(2);  // РАЗМЕР ШРИФТА 2 (КРУПНЫЙ)
  screen.setCursor(25, 5); // СТАВИМ КУРСОР В (25,5)
  screen.print("REACTION"); // ПЕЧАТАЕМ "REACTION"
  
  // РИСУЕМ ПУНКТЫ МЕНЮ МЕЛКИМ ШРИФТОМ
  screen.setTextSize(1);  // РАЗМЕР ШРИФТА 1 (МЕЛКИЙ)
  for(int i = 0; i < 4; i++) {  // ДЛЯ КАЖДОГО ПУНКТА МЕНЮ
    screen.setCursor(10, 25 + i * 10);  // СТАВИМ КУРСОР НА НУЖНУЮ СТРОКУ
    if (i == menuPosition) {
      screen.print("> ");  // ЕСЛИ ЭТО ТЕКУЩИЙ ПУНКТ - РИСУЕМ СТРЕЛКУ
    } else {
      screen.print("  ");  // ЕСЛИ НЕ ТЕКУЩИЙ - ПРОСТО ПРОБЕЛЫ
    }
    screen.print(menuItems[i]);  // ПЕЧАТАЕМ НАЗВАНИЕ ПУНКТА МЕНЮ
  }
  
  // РИСУЕМ ПОДСКАЗКУ ПО УПРАВЛЕНИЮ ВНИЗУ ЭКРАНА
  screen.setCursor(5, 55);  // СТАВИМ КУРСОР ВНИЗУ
  screen.print("");  // ПИШЕМ КАКИЕ КНОПКИ ЧТО ДЕЛАЮТ// не хочу
  
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
}

// ФУНКЦИЯ ДЛЯ ВЫБОРА ПУНКТА МЕНЮ
void selectMenuItem() {
  // В ЗАВИСИМОСТИ ОТ ТЕКУЩЕЙ ПОЗИЦИИ В МЕНЮ ВЫПОЛНЯЕМ РАЗНЫЕ ДЕЙСТВИЯ
  switch(menuPosition) {
    case 0: // NEW GAME - НАЧАТЬ НОВУЮ ИГРУ
      startNewGame();  // ЗАПУСКАЕМ НОВУЮ ИГРУ
      break;
    case 1: // BEST - ПОКАЗАТЬ РЕКОРДЫ И СТАТИСТИКУ
      currentState = BEST_SCREEN;  // ПЕРЕХОДИМ В СОСТОЯНИЕ ЭКРАНА РЕКОРДОВ
      showBestScreen();  // ПОКАЗЫВАЕМ ЭКРАН С РЕКОРДАМИ
      break;
    case 2: // SOURCE - ПОКАЗАТЬ ИНФО ОБ ИСХОДНОМ КОДЕ
      currentState = SOURCE_SCREEN;  // ПЕРЕХОДИМ В СОСТОЯНИЕ ЭКРАНА С ИСХОДНЫМ КОДОМ
      showSourceScreen();  // ПОКАЗЫВАЕМ ЭКРАН С ИНФОЙ О КОДЕ
      break;
    case 3: // ABOUT - ПОКАЗАТЬ ИНФО ОБ ИГРЕ
      currentState = ABOUT_SCREEN;  // ПЕРЕХОДИМ В СОСТОЯНИЕ ЭКРАНА "ОБ ИГРЕ"
      showAboutScreen();  // ПОКАЗЫВАЕМ ЭКРАН "ОБ ИГРЕ"
      break;
  }
}

// ФУНКЦИЯ ДЛЯ НАЧАЛА НОВОЙ ИГРЫ
void startNewGame() {
  points = 0;        // ОБНУЛЯЕМ ОЧКИ
  hardLevel = 1;     // НАЧИНАЕМ С 1 УРОВНЯ
  timeForPress = 1.0; // ДАЕМ 1 СЕКУНДУ НА РЕАКЦИЮ
  currentState = IN_GAME;  // ПЕРЕХОДИМ В СОСТОЯНИЕ ИГРЫ
  showGameStartScreen();  // ПОКАЗЫВАЕМ ЭКРАН НАЧАЛА ИГРЫ
}

// ФУНКЦИЯ ДЛЯ ОТОБРАЖЕНИЯ ЭКРАНА НАЧАЛА ИГРЫ
void showGameStartScreen() {
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  screen.setTextSize(1);  // УСТАНАВЛИВАЕМ МЕЛКИЙ ШРИФТ
  screen.setCursor(40, 15);  // СТАВИМ КУРСОР ДЛЯ НАЗВАНИЯ ИГРЫ
  screen.print("REACTION");  // ПИШЕМ "REACTION"
  screen.setCursor(50, 25);  // СТАВИМ КУРСОР ДЛЯ ПОДЗАГОЛОВКА
  screen.print("TEST");      // ПИШЕМ "TEST"
  
  screen.setTextSize(1);  // МЕЛКИЙ ШРИФТ ДЛЯ ИНСТРУКЦИИ
  screen.setCursor(20, 40);  // СТАВИМ КУРСОР ДЛЯ ПЕРВОЙ СТРОКИ ИНСТРУКЦИИ
  screen.print("Press any button");  // ПИШЕМ "НАЖМИ ЛЮБУЮ КНОПКУ"
  screen.setCursor(30, 50);  // СТАВИМ КУРСОР ДЛЯ ВТОРОЙ СТРОКИ
  screen.print("to start!");  // ПИШЕМ "ЧТОБЫ НАЧАТЬ!"
  
  screen.setCursor(40, 35);  // СТАВИМ КУРСОР ДЛЯ ПОДСКАЗКИ О КНОПКАХ
  screen.print("Btns:1-4");  // ПИШЕМ "КНОПКИ 1-4"
  
  // ЕСЛИ ЕСТЬ РЕКОРД - ПОКАЗЫВАЕМ ЕГО
  if (bestReaction > 0.00) {
    screen.setCursor(25, 60);  // СТАВИМ КУРСОР В САМОМ НИЗУ
    screen.print("Best:");     // ПИШЕМ "РЕКОРД:"
    screen.print(bestReaction, 2);  // ПЕЧАТАЕМ РЕКОРД С 2 ЗНАКАМИ ПОСЛЕ ЗАПЯТОЙ
    screen.print("s");         // ПИШЕМ "С" (СЕКУНДЫ)
  }
  
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
}

// ФУНКЦИЯ ДЛЯ ОБРАБОТКИ НАЧАЛА ИГРЫ (ЭКРАН "PRESS ANY BUTTON")
void handleInGame() {
  // ПРОВЕРЯЕМ КНОПКУ НАЗАД (BTN4) - ЕСЛИ НАЖАТА, ВОЗВРАЩАЕМСЯ В МЕНЮ
  if (digitalRead(BTN4) == LOW) {
    delay(200);  // ЖДЕМ 200 МС ДЛЯ АНТИДРЕБЕЗГА
    returnToMenu();  // ВОЗВРАЩАЕМСЯ В ГЛАВНОЕ МЕНЮ
    return;  // ВЫХОДИМ ИЗ ФУНКЦИИ
  }
  
  // ЕСЛИ НАЖАТА ЛЮБАЯ КНОПКА - НАЧИНАЕМ ИГРУ
  if (digitalRead(BTN1) == LOW || digitalRead(BTN2) == LOW || 
      digitalRead(BTN3) == LOW || digitalRead(BTN4) == LOW) {
    delay(200);  // ЖДЕМ 200 МС ДЛЯ АНТИДРЕБЕЗГА
    currentState = READY_SCREEN;  // ПЕРЕХОДИМ В СОСТОЯНИЕ "ГОТОВЬСЯ"
    readyScreenTime = millis();  // ЗАПОМИНАЕМ КОГДА ПОКАЗАЛИ ЭКРАН "ГОТОВЬСЯ"
    showReadyScreen();  // ПОКАЗЫВАЕМ ЭКРАН "ГОТОВЬСЯ"
  }
}

// ФУНКЦИЯ ДЛЯ ОТОБРАЖЕНИЯ ЭКРАНА "ГОТОВЬСЯ"
void showReadyScreen() {
  randomDelayTime = random(2000, 5001);  // ГЕНЕРИРУЕМ СЛУЧАЙНУЮ ЗАДЕРЖКУ ОТ 2 ДО 5 СЕКУНД
  
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  screen.setTextSize(1);  // УСТАНАВЛИВАЕМ МЕЛКИЙ ШРИФТ
  screen.setCursor(50, 20);  // СТАВИМ КУРСОР ДЛЯ ТЕКСТА "READY?"
  screen.print("READY?");    // ПИШЕМ "READY?"
  
  screen.setTextSize(1);  // МЕЛКИЙ ШРИФТ ДЛЯ ДОПОЛНИТЕЛЬНОГО ТЕКСТА
  screen.setCursor(35, 35);  // СТАВИМ КУРСОР ДЛЯ "Wait..."
  screen.print("Wait...");   // ПИШЕМ "Wait..." (ЖДИ)
  
  screen.setCursor(30, 45);  // СТАВИМ КУРСОР ДЛЯ "Dont press!"
  screen.print("Dont press!"); // ПИШЕМ "Dont press!" (НЕ ЖМИ!)
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
}

// ФУНКЦИЯ ДЛЯ ОБРАБОТКИ ЭКРАНА "ГОТОВЬСЯ"
void handleReadyScreen() {
  // ЕСЛИ ПРОШЛО ДОСТАТОЧНО СЛУЧАЙНОГО ВРЕМЕНИ - НАЧИНАЕМ ИГРУ
  if (millis() - readyScreenTime > randomDelayTime) {
    currentState = GAME_SCREEN;  // ПЕРЕХОДИМ В СОСТОЯНИЕ ИГРОВОГО ЭКРАНА
    startTheGame();  // НАЧИНАЕМ ИГРУ (ПОКАЗЫВАЕМ ЦИФРУ)
  }
  
  // ЕСЛИ ИГРОК НАЖАЛ КНОПКУ СЛИШКОМ РАНО - ПРОИГРЫШ
  if (digitalRead(BTN1) == LOW || digitalRead(BTN2) == LOW || 
      digitalRead(BTN3) == LOW || digitalRead(BTN4) == LOW) {
    gameOver(true);  // ЗАВЕРШАЕМ ИГРУ С ПРИЧИНОЙ "СЛИШКОМ РАНО"
  }
}

// ФУНКЦИЯ ДЛЯ НАЧАЛА ИГРОВОГО РАУНДА (ПОКАЗ ЦИФРЫ)
void startTheGame() {
  startGameTime = millis();  // ЗАПОМИНАЕМ КОГДА НАЧАЛСЯ РАУНД
  needButton = random(0, 4);  // ВЫБИРАЕМ СЛУЧАЙНУЮ КНОПКУ ОТ 0 ДО 3
  
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  
  // РИСУЕМ ТЕКСТ "PRESS:" СВЕРХУ
  screen.setTextSize(1);  // МЕЛКИЙ ШРИФТ
  screen.setCursor(50, 5);  // СТАВИМ КУРСОР ДЛЯ "PRESS:"
  screen.print("PRESS:");   // ПИШЕМ "PRESS:" (ЖМИ:)
  
  // РИСУЕМ БОЛЬШУЮ ЦИФРУ ПОСЕРЕДИНЕ
  screen.setTextSize(2);  // СРЕДНИЙ ШРИФТ ДЛЯ ЦИФРЫ
  screen.setCursor(55, 20);  // СТАВИМ КУРСОР ДЛЯ ЦИФРЫ
  screen.print(buttonText[needButton]);  // ПЕЧАТАЕМ ЦИФРУ КНОПКИ КОТОРУЮ НАДО НАЖАТЬ
  
  // РИСУЕМ ИНФОРМАЦИЮ ОБ ИГРЕ ВНИЗУ ЭКРАНА
  screen.setTextSize(1);  // МЕЛКИЙ ШРИФТ ДЛЯ ИНФОРМАЦИИ
  
  // ЛЕВЫЙ ВЕРХНИЙ УГОЛ - ВРЕМЯ ДЛЯ РЕАКЦИИ
  screen.setCursor(5, 45);  // СТАВИМ КУРСОР В ЛЕВОМ ВЕРХНЕМ УГЛУ
  screen.print("T:");       // ПИШЕМ "T:" (ВРЕМЯ)
  screen.print(timeForPress, 1);  // ПЕЧАТАЕМ ВРЕМЯ С 1 ЗНАКОМ ПОСЛЕ ЗАПЯТОЙ
  screen.print("s");        // ПИШЕМ "S" (СЕКУНДЫ)
  
  // ПРАВЫЙ ВЕРХНИЙ УГОЛ - УРОВЕНЬ СЛОЖНОСТИ
  screen.setCursor(65, 45);  // СТАВИМ КУРСОР В ПРАВОМ ВЕРХНЕМ УГЛУ
  screen.print("L:");        // ПИШЕМ "L:" (УРОВЕНЬ)
  screen.print(hardLevel);   // ПЕЧАТАЕМ ТЕКУЩИЙ УРОВЕНЬ
  
  // ЛЕВЫЙ НИЖНИЙ УГОЛ - ОЧКИ
  screen.setCursor(5, 55);  // СТАВИМ КУРСОР В ЛЕВОМ НИЖНЕМ УГЛУ
  screen.print("S:");       // ПИШЕМ "S:" (ОЧКИ)
  screen.print(points);     // ПЕЧАТАЕМ ТЕКУЩИЕ ОЧКИ
  
  // ПРАВЫЙ НИЖНИЙ УГОЛ - РЕКОРД
  screen.setCursor(65, 55);  // СТАВИМ КУРСОР В ПРАВОМ НИЖНЕМ УГЛУ
  screen.print("B:");        // ПИШЕМ "B:" (РЕКОРД)
  screen.print(bestReaction, 1);  // ПЕЧАТАЕМ РЕКОРД С 1 ЗНАКОМ ПОСЛЕ ЗАПЯТОЙ
  screen.print("s");         // ПИШЕМ "S" (СЕКУНДЫ)
  
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
}

// ФУНКЦИЯ ДЛЯ ОБРАБОТКИ ИГРОВОГО ЭКРАНА (КОГДА ПОКАЗАНА ЦИФРА)
void handleGameScreen() {
  checkButtons();  // ПРОВЕРЯЕМ НАЖАТИЯ КНОПОК
  
  // ЕСЛИ ВРЕМЯ ВЫШЛО И КНОПКУ НЕ НАЖАЛИ - ПРОИГРЫШ
  if (millis() - startGameTime > (timeForPress * 1000)) {
    gameOver(false);  // ЗАВЕРШАЕМ ИГРУ С ПРИЧИНОЙ "ВРЕМЯ ВЫШЛО"
  }
}

// ФУНКЦИЯ ДЛЯ ПРОВЕРКИ НАЖАТИЙ КНОПОК ВО ВРЕМЯ ИГРЫ
void checkButtons() {
  // ЕСЛИ НАЖАТА ПРАВИЛЬНАЯ КНОПКА
  if ((needButton == 0 && digitalRead(BTN1) == LOW) ||
      (needButton == 1 && digitalRead(BTN2) == LOW) ||
      (needButton == 2 && digitalRead(BTN3) == LOW) ||
      (needButton == 3 && digitalRead(BTN4) == LOW)) {
    pressButtonTime = millis();  // ЗАПОМИНАЕМ КОГДА НАЖАЛИ КНОПКУ
    processPress();  // ОБРАБАТЫВАЕМ НАЖАТИЕ (СЧИТАЕМ ВРЕМЯ РЕАКЦИИ)
  }
  // ЕСЛИ НАЖАТА НЕПРАВИЛЬНАЯ КНОПКА
  else if ((needButton != 0 && digitalRead(BTN1) == LOW) ||
           (needButton != 1 && digitalRead(BTN2) == LOW) ||
           (needButton != 2 && digitalRead(BTN3) == LOW) ||
           (needButton != 3 && digitalRead(BTN4) == LOW)) {
    gameOver(false);  // ЗАВЕРШАЕМ ИГРУ С ПРИЧИНОЙ "НЕПРАВИЛЬНАЯ КНОПКА"
  }
}

// ФУНКЦИЯ ДЛЯ ОБРАБОТКИ НАЖАТИЯ КНОПКИ (УСПЕШНОГО ИЛИ НЕТ)
void processPress() {
  // СЧИТАЕМ ВРЕМЯ РЕАКЦИИ В СЕКУНДАХ
  nowReaction = (pressButtonTime - startGameTime) / 1000.0;
  
  // ЕСЛИ УСПЕЛ ВОВРЕМЯ - ПОБЕДА!
  if (nowReaction <= timeForPress) {
    points++;        // ДОБАВЛЯЕМ ОЧКО
    totalWins++;     // УВЕЛИЧИВАЕМ СЧЕТЧИК ПОБЕД
    totalPoints += points;  // ДОБАВЛЯЕМ ОЧКИ К ОБЩЕЙ СУММЕ
    
    // ЕСЛИ ЭТО НОВЫЙ РЕКОРД - ОБНОВЛЯЕМ ЕГО
    if (nowReaction < bestReaction || bestReaction == 0.00) {
      bestReaction = nowReaction;  // ЗАПОМИНАЕМ НОВЫЙ РЕКОРД
      saveAllData();  // СОХРАНЯЕМ В ПАМЯТЬ ESP32
    }
    
    makeHarder();  // УВЕЛИЧИВАЕМ СЛОЖНОСТЬ
    
    // ПЕРЕХОДИМ НА ЭКРАН ПОБЕДЫ
    currentState = WIN_SCREEN;
    showWinScreen();
  } else {
    // ЕСЛИ НЕ УСПЕЛ ВОВРЕМЯ - ПРОИГРЫШ
    gameOver(false);
  }
}

// ФУНКЦИЯ ДЛЯ УВЕЛИЧЕНИЯ СЛОЖНОСТИ ИГРЫ
void makeHarder() {
  // В ЗАВИСИМОСТИ ОТ КОЛИЧЕСТВА ОЧКОВ УМЕНЬШАЕМ ВРЕМЯ И УВЕЛИЧИВАЕМ УРОВЕНЬ
  if (points <= 10) {
    timeForPress = 1.00;  // 1 СЕКУНДА
    hardLevel = 1;        // УРОВЕНЬ 1
  } else if (points <= 20) {
    timeForPress = 0.80;  // 0.8 СЕКУНДЫ
    hardLevel = 2;        // УРОВЕНЬ 2
  } else if (points <= 30) {
    timeForPress = 0.60;  // 0.6 СЕКУНДЫ
    hardLevel = 3;        // УРОВЕНЬ 3
  } else if (points <= 40) {
    timeForPress = 0.45;  // 0.45 СЕКУНДЫ
    hardLevel = 4;        // УРОВЕНЬ 4
  } else if (points <= 50) {
    timeForPress = 0.35;  // 0.35 СЕКУНДЫ
    hardLevel = 5;        // УРОВЕНЬ 5
  } else if (points <= 60) {
    timeForPress = 0.25;  // 0.25 СЕКУНДЫ
    hardLevel = 6;        // УРОВЕНЬ 6
  } else if (points <= 70) {
    timeForPress = 0.18;  // 0.18 СЕКУНДЫ
    hardLevel = 7;        // УРОВЕНЬ 7
  } else if (points <= 80) {
    timeForPress = 0.12;  // 0.12 СЕКУНДЫ
    hardLevel = 8;        // УРОВЕНЬ 8
  } else if (points <= 90) {
    timeForPress = 0.08;  // 0.08 СЕКУНДЫ
    hardLevel = 9;        // УРОВЕНЬ 9
  } else if (points <= 100) {
    timeForPress = 0.05;  // 0.05 СЕКУНДЫ
    hardLevel = 10;       // УРОВЕНЬ 10
  } else {
    timeForPress = 0.03;  // 0.03 СЕКУНДЫ (ОЧЕНЬ СЛОЖНО!)
    hardLevel = 11;       // УРОВЕНЬ 11 (МАКСИМАЛЬНЫЙ)
  }
}

// ФУНКЦИЯ ДЛЯ ОТОБРАЖЕНИЯ ЭКРАНА ПОБЕДЫ
void showWinScreen() {
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  screen.setTextSize(1);  // УСТАНАВЛИВАЕМ МЕЛКИЙ ШРИФТ
  screen.setCursor(45, 15);  // СТАВИМ КУРСОР ДЛЯ "SUCCESS!"
  screen.print("SUCCESS!");  // ПИШЕМ "SUCCESS!" (УСПЕХ!)
  
  screen.setTextSize(1);  // МЕЛКИЙ ШРИФТ ДЛЯ СТАТИСТИКИ
  screen.setCursor(20, 30);  // СТАВИМ КУРСОР ДЛЯ ВРЕМЕНИ РЕАКЦИИ
  screen.print("T:");        // ПИШЕМ "T:" (ВРЕМЯ)
  screen.print(nowReaction, 2);  // ПЕЧАТАЕМ ВРЕМЯ РЕАКЦИИ С 2 ЗНАКАМИ
  screen.print("s");         // ПИШЕМ "S" (СЕКУНДЫ)
  
  screen.setCursor(20, 40);  // СТАВИМ КУРСОР ДЛЯ ОЧКОВ
  screen.print("S:");        // ПИШЕМ "S:" (ОЧКИ)
  screen.print(points);      // ПЕЧАТАЕМ ТЕКУЩИЕ ОЧКИ
  
  screen.setCursor(20, 50);  // СТАВИМ КУРСОР ДЛЯ УРОВНЯ
  screen.print("L:");        // ПИШЕМ "L:" (УРОВЕНЬ)
  screen.print(hardLevel);   // ПЕЧАТАЕМ ТЕКУЩИЙ УРОВЕНЬ
  
  // ЕСЛИ ЭТО НОВЫЙ РЕКОРД - ПОКАЗЫВАЕМ СООБЩЕНИЕ
  if (nowReaction == bestReaction) {
    screen.setCursor(25, 25);  // СТАВИМ КУРСОР ДЛЯ "NEW RECORD!"
    screen.print("NEW RECORD!"); // ПИШЕМ "NEW RECORD!" (НОВЫЙ РЕКОРД!)
  }
  
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
}

// ФУНКЦИЯ ДЛЯ ОБРАБОТКИ ЭКРАНА ПОБЕДЫ
void handleWinScreen() {
  // ЖДЕМ 1.5 СЕКУНДЫ ЧТОБЫ ИГРОК УВИДЕЛ СВОЙ УСПЕХ
  delay(1500);
  
  // ПРОДОЛЖАЕМ ИГРУ - ПЕРЕХОДИМ К СЛЕДУЮЩЕМУ РАУНДУ
  currentState = READY_SCREEN;  // ПЕРЕХОДИМ В СОСТОЯНИЕ "ГОТОВЬСЯ"
  readyScreenTime = millis();   // ЗАПОМИНАЕМ КОГДА ПОКАЗАЛИ ЭКРАН "ГОТОВЬСЯ"
  showReadyScreen();            // ПОКАЗЫВАЕМ ЭКРАН "ГОТОВЬСЯ"
}

// ФУНКЦИЯ ДЛЯ ЗАВЕРШЕНИЯ ИГРЫ (ПРОИГРЫША)
void gameOver(bool tooEarly) {
  totalGames++;        // УВЕЛИЧИВАЕМ СЧЕТЧИК СЫГРАННЫХ ИГР
  currentState = GAME_OVER;  // ПЕРЕХОДИМ В СОСТОЯНИЕ ПРОИГРЫША
  
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  screen.setTextSize(1);  // УСТАНАВЛИВАЕМ МЕЛКИЙ ШРИФТ
  screen.setCursor(40, 10);  // СТАВИМ КУРСОР ДЛЯ "GAME OVER"
  screen.print("GAME OVER");  // ПИШЕМ "GAME OVER" (ИГРА ОКОНЧЕНА)
  
  screen.setTextSize(1);  // МЕЛКИЙ ШРИФТ ДЛЯ СТАТИСТИКИ
  screen.setCursor(10, 25);  // СТАВИМ КУРСОР ДЛЯ ОЧКОВ
  screen.print("S:");        // ПИШЕМ "S:" (ОЧКИ)
  screen.print(points);      // ПЕЧАТАЕМ ТЕКУЩИЕ ОЧКИ
  
  screen.setCursor(70, 25);  // СТАВИМ КУРСОР ДЛЯ УРОВНЯ
  screen.print("L:");        // ПИШЕМ "L:" (УРОВЕНЬ)
  screen.print(hardLevel);   // ПЕЧАТАЕМ ТЕКУЩИЙ УРОВЕНЬ
  
  screen.setCursor(10, 35);  // СТАВИМ КУРСОР ДЛЯ РЕКОРДА
  screen.print("B:");        // ПИШЕМ "B:" (РЕКОРД)
  screen.print(bestReaction, 2);  // ПЕЧАТАЕМ РЕКОРД С 2 ЗНАКАМИ
  screen.print("s");         // ПИШЕМ "S" (СЕКУНДЫ)
  
  // РИСУЕМ ОБЩУЮ СТАТИСТИКУ ВНИЗУ ЭКРАНА
  screen.setCursor(5, 55);  // СТАВИМ КУРСОР ВНИЗУ
  screen.print("G:");       // ПИШЕМ "G:" (ИГРЫ)
  screen.print(totalGames); // ПЕЧАТАЕМ ОБЩЕЕ КОЛИЧЕСТВО ИГР
  screen.print(" W:");      // ПИШЕМ "W:" (ПОБЕДЫ)
  screen.print(totalWins);  // ПЕЧАТАЕМ КОЛИЧЕСТВО ПОБЕД
  
  // ПИШЕМ ПРИЧИНУ ПРОИГРЫША В ЗАВИСИМОСТИ ОТ ПАРАМЕТРА
  if (tooEarly) {
    screen.setCursor(30, 45);  // СТАВИМ КУРСОР ДЛЯ "TOO EARLY!"
    screen.print("TOO EARLY!"); // ПИШЕМ "TOO EARLY!" (СЛИШКОМ РАНО!)
  } else if (nowReaction > timeForPress) {
    screen.setCursor(35, 45);  // СТАВИМ КУРСОР ДЛЯ "TOO SLOW!"
    screen.print("TOO SLOW!");  // ПИШЕМ "TOO SLOW!" (СЛИШКОМ МЕДЛЕННО!)
  } else {
    screen.setCursor(30, 45);  // СТАВИМ КУРСОР ДЛЯ "WRONG BTN!"
    screen.print("WRONG BTN!"); // ПИШЕМ "WRONG BTN!" (НЕПРАВИЛЬНАЯ КНОПКА!)
  }
  
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
  
  saveAllData();  // СОХРАНЯЕМ ВСЕ ДАННЫЕ В ПАМЯТЬ ESP32
}

// ФУНКЦИЯ ДЛЯ ОБРАБОТКИ ЭКРАНА ПРОИГРЫША
void handleGameOver() {
  // ЕСЛИ НАЖАТА ЛЮБАЯ КНОПКА - ВОЗВРАЩАЕМСЯ В МЕНЮ
  if (digitalRead(BTN1) == LOW || digitalRead(BTN2) == LOW || 
      digitalRead(BTN3) == LOW || digitalRead(BTN4) == LOW) {
    delay(200);  // ЖДЕМ 200 МС ДЛЯ АНТИДРЕБЕЗГА
    returnToMenu();  // ВОЗВРАЩАЕМСЯ В ГЛАВНОЕ МЕНЮ
  }
}

// ФУНКЦИЯ ДЛЯ ОБРАБОТКИ ИНФОРМАЦИОННЫХ ЭКРАНОВ (BEST, SOURCE, ABOUT)
void handleInfoScreens() {
  // ЕСЛИ НАЖАТА КНОПКА НАЗАД (BTN4) - ВОЗВРАЩАЕМСЯ В МЕНЮ
  if (digitalRead(BTN4) == LOW) {
    delay(200);  // ЖДЕМ 200 МС ДЛЯ АНТИДРЕБЕЗГА
    returnToMenu();  // ВОЗВРАЩАЕМСЯ В ГЛАВНОЕ МЕНЮ
  }
}

// ФУНКЦИЯ ДЛЯ ОТОБРАЖЕНИЯ ЭКРАНА С РЕКОРДАМИ И СТАТИСТИКОЙ
void showBestScreen() {
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  
  // РИСУЕМ ЗАГОЛОВОК "BEST" КРУПНЫМ ШРИФТОМ
  screen.setTextSize(2);  // РАЗМЕР ШРИФТА 2 (КРУПНЫЙ)
  screen.setCursor(40, 5);  // СТАВИМ КУРСОР ДЛЯ "BEST"
  screen.print("BEST");     // ПИШЕМ "BEST" (РЕКОРДЫ)
  
  // РИСУЕМ СТАТИСТИКУ МЕЛКИМ ШРИФТОМ
  screen.setTextSize(1);  // РАЗМЕР ШРИФТА 1 (МЕЛКИЙ)
  screen.setCursor(10, 25);  // СТАВИМ КУРСОР ДЛЯ ЛУЧШЕГО ВРЕМЕНИ
  screen.print("Best Time: ");  // ПИШЕМ "ЛУЧШЕЕ ВРЕМЯ: "
  screen.print(bestReaction, 2);  // ПЕЧАТАЕМ ЛУЧШЕЕ ВРЕМЯ С 2 ЗНАКАМИ
  screen.print("s");         // ПИШЕМ "S" (СЕКУНДЫ)
  
  screen.setCursor(10, 35);  // СТАВИМ КУРСОР ДЛЯ КОЛИЧЕСТВА ИГР
  screen.print("Total Games: ");  // ПИШЕМ "ВСЕГО ИГР: "
  screen.print(totalGames);  // ПЕЧАТАЕМ КОЛИЧЕСТВО ИГР
  
  screen.setCursor(10, 45);  // СТАВИМ КУРСОР ДЛЯ СТАТИСТИКИ ПОБЕД
  screen.print("Wins: ");    // ПИШЕМ "ПОБЕДЫ: "
  screen.print(totalWins);   // ПЕЧАТАЕМ КОЛИЧЕСТВО ПОБЕД
  screen.print("/");         // ПИШЕМ "/"
  screen.print(totalGames);  // ПЕЧАТАЕМ ОБЩЕЕ КОЛИЧЕСТВО ИГР
  
  // ЕСЛИ БЫЛИ ИГРЫ - СЧИТАЕМ И ВЫВОДИМ ПРОЦЕНТ ПОБЕД
  if (totalGames > 0) {
    screen.setCursor(10, 55);  // СТАВИМ КУРСОР ДЛЯ ПРОЦЕНТА ПОБЕД
    screen.print("Win Rate: ");  // ПИШЕМ "ПРОЦЕНТ ПОБЕД: "
    int percent = (totalWins * 100) / totalGames;
    screen.print(percent);
  }
  
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
}

// ФУНКЦИЯ ДЛЯ ОТОБРАЖЕНИЯ ЭКРАНА С ИНФОРМАЦИЕЙ ОБ ИСХОДНОМ КОДЕ
void showSourceScreen() {
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  
  // РИСУЕМ ЗАГОЛОВОК "SOURCE" КРУПНЫМ ШРИФТОМ
  screen.setTextSize(2);  // РАЗМЕР ШРИФТА 2 (КРУПНЫЙ)
  screen.setCursor(30, 5);  // СТАВИМ КУРСОР ДЛЯ "SOURCE"
  screen.print("SOURCE");   // ПИШЕМ "SOURCE" (ИСХОДНЫЙ КОД)
  
  // РИСУЕМ ИНФОРМАЦИЮ О КОДЕ МЕЛКИМ ШРИФТОМ
  screen.setTextSize(1);  // РАЗМЕР ШРИФТА 1 (МЕЛКИЙ)
  screen.setCursor(5, 25);  // СТАВИМ КУРСОР ДЛЯ ПЕРВОЙ СТРОКИ
  screen.print("GitHub:");  // ПИШЕМ "GitHub:"
  screen.setCursor(5, 35);  // СТАВИМ КУРСОР ДЛЯ ВТОРОЙ СТРОКИ
  screen.print("github.com/lakladon");  // ПИШЕМ АДРЕС НА GITHUB
  screen.setCursor(5, 45);  // СТАВИМ КУРСОР ДЛЯ ТРЕТЬЕЙ СТРОКИ
  screen.print("/reaction-game");  // ПИШЕМ НАЗВАНИЕ РЕПОЗИТОРИЯ
  
  screen.setCursor(5, 55);  // СТАВИМ КУРСОР ДЛЯ ЧЕТВЕРТОЙ СТРОКИ
  screen.print("ESP32+SSD1306");  // ПИШЕМ ТЕХНОЛОГИИ
  
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
}

// ФУНКЦИЯ ДЛЯ ОТОБРАЖЕНИЯ ЭКРАНА "ОБ ИГРЕ"
void showAboutScreen() {
  screen.clearDisplay();  // ОЧИЩАЕМ ЭКРАН
  
  // РИСУЕМ ЗАГОЛОВОК "ABOUT" КРУПНЫМ ШРИФТОМ
  screen.setTextSize(2);  // РАЗМЕР ШРИФТА 2 (КРУПНЫЙ)
  screen.setCursor(35, 5);  // СТАВИМ КУРСОР ДЛЯ "ABOUT"
  screen.print("ABOUT");    // ПИШЕМ "ABOUT" (ОБ ИГРЕ)
  
  // РИСУЕМ ИНФОРМАЦИЮ ОБ ИГРЕ МЕЛКИМ ШРИФТОМ
  screen.setTextSize(1);  // РАЗМЕР ШРИФТА 1 (МЕЛКИЙ)
  screen.setCursor(10, 25);  // СТАВИМ КУРСОР ДЛЯ ПЕРВОЙ СТРОКИ
  screen.print("Reaction Test Game");  // ПИШЕМ НАЗВАНИЕ ИГРЫ
  screen.setCursor(10, 35);  // СТАВИМ КУРСОР ДЛЯ ВТОРОЙ СТРОКИ
  screen.print("By: LAKLADON");  // ПИШЕМ АВТОРА
  screen.setCursor(10, 45);  // СТАВИМ КУРСОР ДЛЯ ТРЕТЬЕЙ СТРОКИ
  screen.print("Version: 3.0");  // ПИШЕМ ВЕРСИЮ
  screen.setCursor(10, 55);  // СТАВИМ КУРСОР ДЛЯ ЧЕТВЕРТОЙ СТРОКИ
  screen.print("ESP32 + OLED 128x64");  // ПИШЕМ ОБОРУДОВАНИЕ
  
  screen.display();  // ПОКАЗЫВАЕМ ВСЕ НА ЭКРАНЕ
}

// ФУНКЦИЯ ДЛЯ ВОЗВРАТА В ГЛАВНОЕ МЕНЮ
void returnToMenu() {
  currentState = MAIN_MENU;  // УСТАНАВЛИВАЕМ СОСТОЯНИЕ "ГЛАВНОЕ МЕНЮ"
  showMainMenu();            // ПОКАЗЫВАЕМ ГЛАВНОЕ МЕНЮ
}
