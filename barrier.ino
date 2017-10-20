#include <iarduino_RTC.h>
iarduino_RTC time(RTC_DS1307); // подключаем RTC модуль на базе чипа DS1307, используется аппаратная шина I2C

//Время "нажатия на кнопку" для поднятия стрелы
#define OPEN_ARROW_BUTTON_DELAY     500

//Время ожидания поднятия стрелы
#define OPENING_ARROW_DELAY         8000

#define START_MOSFET_PIN    6
#define EMRG_MOSFET_PIN    7

#define SW_CLOSE_PIN    8
#define SW_OPEN_PIN     9

enum ArrowStates {CLOSE, OPEN, MIDDLE, BOTH_OPEN};
int currentArrowState;

int DAY_START_HOUR = 7;
int DAY_START_MIN = 30;
int DAY_START_SEC = 0;

int DAY_END_HOUR = 21;
int DAY_END_MIN = 30;
int DAY_END_SEC = 0;


void setup() {
  Serial.begin(9600);
  time.begin();
  //time.settime(50,29,21,17,10,17,2);  // 0  сек, 51 мин, 21 час, 27, октября, 2015 года, вторник
  time.settime(50,29,7,17,10,17,2);  // 0  сек, 51 мин, 21 час, 27, октября, 2015 года, вторник
    
  pinMode(LED_BUILTIN, OUTPUT);

  //Мосфет на EMRG должен быть по умолчанию замкнут
  pinMode(EMRG_MOSFET_PIN, OUTPUT);  
  digitalWrite(EMRG_MOSFET_PIN, HIGH);

  //Мосфет на открытие шлакбаума должен быть по умолчанию разомкнут
  pinMode(START_MOSFET_PIN, OUTPUT);  
  digitalWrite(START_MOSFET_PIN, LOW);

  //Концевики  
  pinMode(SW_CLOSE_PIN, INPUT);
  pinMode(SW_OPEN_PIN, INPUT);

  Serial.println(time.gettime("d-m-Y, H:i:s, D"));
  currentArrowState = getArrowState();
  printArrowState(currentArrowState);
  
  //Определяем время суток и применяем программу
  time.gettime();
  if(isNowDay()){    
   Serial.println("Start state: DAY"); 
   applyDay();
  }
  else{    
   Serial.println("Start state: NIGHT"); 
   applyNight();
  }    
}

void loop() {   
  if(millis()%1000==0){ // если прошла 1 секунда
    Serial.println(time.gettime("d-m-Y, H:i:s, D"));
    
    //Вывод состояния стрелы, при изменении
    int newArrowState = getArrowState();
    if(currentArrowState != newArrowState){
      currentArrowState = newArrowState;    
      printArrowState(currentArrowState);
    }  
        
    time.gettime();

    //Время начала НОЧИ
    if(isNowNightStartTime()){
      Serial.println("Night START"); 
      applyNight();    
    }
      
    
    //Время начала ДНЯ
    if(isNowDayStartTime()){
      Serial.println("Day START");   
      applyDay();      
    }
    
  }
}

void applyNight(){
  Serial.println("applyNight"); 
  
  int arrowState = getArrowState();
  printArrowState(arrowState);
  
  //Если стрела опущена 
  if(arrowState == CLOSE || arrowState == MIDDLE){
    openArrow(); //Поднимаем стрелу        
    delay(OPENING_ARROW_DELAY); //Ждем пока она откроется        
    arrowState = getArrowState(); //Обновляем состояние стрелы                
    printArrowState(arrowState);
  }      
  
  //Если стрела поднята
  if(arrowState == OPEN){        
    //Размыкаем контакты EMRG
    breakEMRG();
  }else{
    Serial.println("Error: Arrow NOT OPEN!"); 
  }
}

void applyDay(){
    Serial.println("applyDay");      
  //Замыкаем контакты EMRG
  repairEMRG();
}

bool isNowDayStartTime(){
  if(time.Hours == DAY_START_HOUR && time.minutes == DAY_START_MIN && time.seconds == DAY_START_SEC)
    return true;  
  else
   return false;
}

bool isNowNightStartTime(){
  if(time.Hours == DAY_END_HOUR && time.minutes == DAY_END_MIN && time.seconds == DAY_END_SEC)
    return true;  
  else
   return false;
}

//Определяет время суток
bool isNowDay(){
     
  //Если часы в нужном диапазоне
  if(time.Hours >= DAY_START_HOUR && time.Hours <= DAY_END_HOUR){

      //Если это последний час дня
      if(time.Hours == DAY_END_HOUR){
        
        //сравнить минуты
        if(time.minutes < DAY_END_MIN)
          return true;
         else
          return false;        
      }
      
      //Если это первый час дня
      if(time.Hours == DAY_START_HOUR){
        //сравнить минуты
        if(time.minutes >= DAY_START_MIN)
          return true;
         else
          return false;  
      }

      //Если это ни первый ни последний час
      return true;

  }else{ //Нет... значит это ночь
    return false;
  }
  

}

void printArrowState(int arrowState){
    String stringArrowState;
    switch(arrowState){
      case OPEN:
        stringArrowState = "OPEN";
      break;
      case CLOSE:
        stringArrowState = "CLOSE";
      break;
      case MIDDLE:
        stringArrowState = "MIDDLE";
      break;
      case BOTH_OPEN:
        stringArrowState = "BOTH_OPEN";
      break;
    }
    Serial.print("Arrow state: ");
    Serial.print(stringArrowState);
    Serial.println("");  
}

//Разрывает выход EMRG
void breakEMRG(){
  digitalWrite(EMRG_MOSFET_PIN, LOW);
  Serial.println("breakEMRG");
}

//Сращивает выход EMRG
void repairEMRG(){
  digitalWrite(EMRG_MOSFET_PIN, HIGH);
  Serial.println("repairEMRG");
}

//Открывает шлакбаум
void openArrow(){  
  digitalWrite(START_MOSFET_PIN, HIGH);
  delay(OPEN_ARROW_BUTTON_DELAY);
  digitalWrite(START_MOSFET_PIN, LOW);
  Serial.println("OPENING Arrow...");
}

/*
 *  Выдает состояние стрелы
 *  OPEN - открыта
 *  CLOSE - закрыта
 *  MIDDLE - по середине и не открыта и не закрыта
 *  BOTH_OPEN - ошибка (и открыта и закрыта)
 *  
 *  Когда на концевике +5 то он в КРАЙНЕМ положении 
 *  в остальных случиях GND
*/
int getArrowState(){    
  bool close = digitalRead(SW_CLOSE_PIN);
  bool open  = digitalRead(SW_OPEN_PIN);
  
  if(open && !close)
    return OPEN;

  if(!open && close)
    return CLOSE;

  if(!open && !close)
    return MIDDLE;

  return BOTH_OPEN;
}

