#include <Arduino.h>
#include <ESP32Lib.h>
#include <Ressources/Font8x8.h>

// piny VGA
const int redPin = 21;
const int greenPin = 19;
const int bluePin = 18;
const int hSyncPin = 16;
const int vSyncPin = 17;

const int xWidth = 10; // kolumny
const int yWidth = 20; // wiersze

// https://tetris.fandom.com/wiki/Tetromino
const char tetrominoChars[] = {'I', 'J', 'L', 'O', 'S', 'T', 'Z'};
const int8_t tetrominoMatrixes[7][4][4] = {
    // I
    {
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
    },
    // J
    {
        {0, 0, 0, 0},
        {1, 0, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0},
    },
    // L
    {
        {0, 0, 0, 0},
        {0, 0, 1, 0},
        {1, 1, 1, 0},
        {0, 0, 0, 0},
    },
    // O
    {
        {0, 0, 0, 0},
        {1, 1, 0, 0},
        {1, 1, 0, 0},
        {0, 0, 0, 0},
    },
    // S
    {
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {1, 1, 0, 0},
        {0, 0, 0, 0},
    },
    // T
    {
        {0, 0, 0, 0},
        {1, 1, 1, 0},
        {0, 1, 0, 0},
        {0, 0, 0, 0},
    },
    // Z
    {
        {0, 0, 0, 0},
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0},
    }
};
const int tetrominoColors[7][3] = {
    {0, 255, 255}, // I
    {0, 0, 255}, // J
    {255, 255, 255}, // L
    {255, 255, 0}, // O
    {0, 255, 0}, // S
    {128, 0, 128}, // T
    {255, 0, 0}, // Z
};

TaskHandle_t DrawInput;
TaskHandle_t Update;

VGA3Bit vga;

// pole gry
int8_t playfield[yWidth][xWidth];

// punktacja
// 100 punktów za wiersz
//TODO: zrobić pełny system punktacji
int score;

// nie wiem czy jakkolwiek polepszy to wydajność
// ale tylko jeden obiekt Tetromino jest tworzony
// i będzie on używany wielokrotnie
class Tetromino {
public:
  char type; // typ np. I lub J
  int8_t typeIndex;
  int8_t matrix[4][4];
  int x; // kolumna
  int y; // wiersz

  void getNewTetromino() {
    Serial.println("=== getNewTetromino ===");
    x = 5;
    y = 0;
    typeIndex = random(0, 7);
    type = tetrominoChars[typeIndex];
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        Serial.print(tetrominoMatrixes[typeIndex][i][j]);
        matrix[i][j] = tetrominoMatrixes[typeIndex][i][j];
      }
      Serial.println();
    }
  }

  Tetromino() {
    getNewTetromino();
  }

  // rysowanie tetromino do tylnego bufora
  void draw(){
    Serial.println("=== draw ===");
    Serial.print("x: ");
    Serial.println(x);
    Serial.print("y: ");
    Serial.println(y);
    Serial.print("type: ");
    Serial.println(type);
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        if (matrix[i][j] == 1) {
          vga.fillRect(
            ( x + j) * 6,
            (y + i) * 6,
            5,
            5,
            vga.RGB(
            tetrominoColors[typeIndex][0],
            tetrominoColors[typeIndex][1],
            tetrominoColors[typeIndex][2])
          );
        }
      }
    }
  }

  // przenoszenie tetromino na pole gry
  void place(){
    Serial.println("=== place ===");
    // przenieś tetromino na pole gry
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        if (matrix[i][j] == 1) {
          playfield[y + i][x + j] = typeIndex + 1;
        }
      }
    }

    // sprawdzanie czy wiersz jest wypełniony
    for (int j = 0; j < yWidth; j++) {
      bool lineClear = true;
      for (int i = 0; i < xWidth; i++) {
        if (playfield[j][i] == 0) {
          lineClear = false;
          break;
        }
      }
      if (lineClear) {
        // dodaj 100 punktów
        score += 100;
        // przenieś wszystkie linie powyżej wiersza j o jeden w dół
        for (int k = j; k > 0; k--) {
          for (int i = 0; i < xWidth; i++) {
            playfield[k][i] = playfield[k - 1][i];
          }
        }
      }
    }

    // stwórz nowe tetromino
    getNewTetromino();
  }

  // aktualizowanie pozycji tetromino
  void update(){
    Serial.println("=== update ===");
    // sprawdzanie czy ruch jest możliwy
    bool valid = true;
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        if (matrix[i][j] == 1) {
          if (y + i + 1 >= yWidth) {
            valid = false;
            break;
          }
          if (playfield[y + i + 1][x + j] != 0) {
            valid = false;
            break;
          }
        }
      }
    }

    if (!valid) { // jeśli ruch jest możliwy to umieść tetromino na polu gry
      Serial.println("place");
      place();
    } else { // jeśli ruch jest możliwy to przesuń tetromino w dół
      Serial.println("move down");
      y++;
    }
  }

  // obracanie tetromino
  void rotate(){
    // sprawdzanie czy ruch jest możliwy
    bool valid = true;
    int8_t tempMatrix[4][4];
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        tempMatrix[i][j] = matrix[i][j];
      }
    }
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        matrix[i][j] = tempMatrix[3 - j][i];
      }
    }
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        if (matrix[i][j] == 1) {
          if (x + j < 0 || x + j >= xWidth) {
            valid = false;
            break;
          }
          if (y + i >= yWidth) {
            valid = false;
            break;
          }
          if (playfield[y + i][x + j] != 0) {
            valid = false;
            break;
          }
        }
      }
    }

    if (!valid) { // jeśli ruch jest niemożliwy to przywróć poprzednią macierz
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
          matrix[i][j] = tempMatrix[i][j];
        }
      }
    }
  }
};

Tetromino tetromino;

void updateLoop(void * pvParameters) {
  while (true) {
    tetromino.update();

    delay(1000);
  }
}

void drawInputLoop(void * pvParameters) {
  while (true) {
    vga.clear();

    while(Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'a') {
        // sprawdzanie czy ruch jest możliwy
        // jeśli tak to przesuń tetromino w lewo
        bool valid = true;
        for (int i = 0; i < 4; i++) {
          for (int j = 0; j < 4; j++) {
            if (tetromino.matrix[i][j] == 1) {
              if (tetromino.x + j - 1 < 0) {
                valid = false;
                break;
              }
              if (playfield[tetromino.y + i][tetromino.x + j - 1] != 0) {
                valid = false;
                break;
              }
            }
          }
        }
        
        if (valid) {
          tetromino.x--;
        }
      } else if (c == 'd') {
        // sprawdzanie czy ruch jest możliwy
        // jeśli tak to przesuń tetromino w prawo
        bool valid = true;
        for (int i = 0; i < 4; i++) {
          for (int j = 0; j < 4; j++) {
            if (tetromino.matrix[i][j] == 1) {
              if (tetromino.x + j + 1 >= xWidth) {
                valid = false;
                break;
              }
              if (playfield[tetromino.y + i][tetromino.x + j + 1] != 0) {
                valid = false;
                break;
              }
            }
          }
        }

        if (valid) {
          tetromino.x++;
        }
      } else if (c == 'w') {
        // obracanie tetromino
        tetromino.rotate();
      }
    }

    // wyswietlanie obramowania pola gry
    vga.line(0, 0, xWidth * 6, 0, vga.RGB(255, 255, 255));
    vga.line(0, 0, 0, yWidth * 6, vga.RGB(255, 255, 255));
    vga.line(xWidth * 6, 0, xWidth * 6, yWidth * 6, vga.RGB(255, 255, 255));
    vga.line(0, yWidth * 6, xWidth * 6, yWidth * 6, vga.RGB(255, 255, 255));

    // wyświetlanie tetromino
    tetromino.draw();

    // wyświetlanie pola gry
    for (int i = 0; i < xWidth; i++) {
      for (int j = 0; j < yWidth; j++) {
        if (playfield[j][i] != 0) {
          vga.fillRect(
            i * 6,
            j * 6,
            5,
            5,
            vga.RGB(
            tetrominoColors[playfield[j][i] - 1][0],
            tetrominoColors[playfield[j][i] - 1][1],
            tetrominoColors[playfield[j][i] - 1][2])
          );
        }
      }
    }

    // wyświetlanie wyniku
    vga.setCursor(100, 20);
    vga.setTextColor(vga.RGB(255, 255, 255));
    vga.print("Wynik: ");
    vga.print(score);

    // zmień wyświetlany bufor
    vga.show();

    delay(100);
  }
}

void setup() {
  Serial.begin(115200);

  // potrzebne do losowania bloków
  srand(time(NULL));

  // podwójny bufor dla płynnych animacji
  vga.setFrameBufferCount(2);

  // ustawienie pinów i rozdzielczości
  vga.init(vga.MODE320x200, redPin, greenPin, bluePin, hSyncPin, vSyncPin);

  // ustawienie czcionki
  vga.setFont(Font8x8);

  tetromino = Tetromino();

  // wyświetlanie oraz osługa wejścia
  xTaskCreatePinnedToCore(drawInputLoop, "DrawInput", 10000, NULL,  1, &DrawInput, 1);

  delay(100);

  // aktualizacja pozyji tetromino
  xTaskCreatePinnedToCore(updateLoop, "Update", 10000, NULL, 1, &Update, 0);

  delay(100);            
}

void loop() {
  
} 