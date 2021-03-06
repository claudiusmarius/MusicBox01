  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Claude DUFOURMONT Le 06/01/2022
  // Réalisation d'une boîte à musique avec animation lumineuse
  // Correction  logarithmique du volume par code
  // Rajout de tempo pour coupure d'alimentation : MosfetGate passe à LOW dés que le temps écoulé est supérieur à interval
  // La tempo fonctionne avec une animation lumineuse, mais avec une durée supplémentaire imputable au déroulement del'animation
  // La tempo est réglable via un potentiomètre et un générateur de fonction 3 segments, segment1 : de 1 à 6mn - zone2 : de 6 à 60mn - segment3 : de 60 à 600mn
  // segment1: LED bleue - segment2 : LED bleue et jaune - segment3 : LED jaune 
  // MosfetGate vient agir sur un BD232 (BJT NPN Puissance)via un optocoupleur pour couper le VCC des matrices => Ok (ne marche pas par coupure 0V),ce n'est pas l'isolement
  // galvanique qui est recherché, c'est une astuce de circuiterie
  // MosfetGate vient agir sur Mosfet canal N directement pour couper le 0V du DFPlayer-Mini ==> Ok
  // Une résistance associée à une zener de 3.3v (triée pour être légérement inférieure), permet d'adapter le niveau 5V ATtiny85 avec le niveau 3.3V de la puce du DFPlayer -Mini
  // Ajout de plusieurs scenariis d'animation lumineuse, bien sur le temps augmente
  // Rajout d'un BP pour positionner la sortie MosfetGate à zéro
  // Rajout d'un BP pour actionner le DFPlayer-Mini sur pause
  // Rajout d'un BP pour actionner le DFPlayer-Mini sur start
  // L'essai n'a pas été concluant avec des interruptions sur les falling des BP ==> le test BP est réalisé régulièrement dans le code (un peu pifométriquement, mais ça marche)
  // Mise en place de 4 condensateurs (20nF) et résistances PullUp (12K) pour éviter les rebonds
  // Rajout d'un fil par soudure sur le Pad RESET : Attention fil souple uniquement, si c'est un PINHEADER, le Pad sera arraché en cas de flexion
  // Seeeduino XIAO et ATtiny85 + DFPlayer mini
  // Schéma utilisé : en cours de mise à jour.
  // Le potar son est pris en compte à chaque redémarrage uniquement
  // Le code est prévu pour 1/3 dossier et est pris en compte au redémarrage uniquement (plus est possible avec modification)
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  
  #include <Adafruit_NeoPixel.h>
    
  #define BrocheDAC A0                                            // Envoie signal ANA vers ATtiny85 pour discriminer Pause/Start DFPlayer-Mini
  #define potPinInterval A4                                       // RP2 10K

  #define BrocheBuzzer 1
  int ValueDAC;                                                   // Echelle : 0 -> 1023 points (analogWriteResolution(10)) pour 0 -> 3.3v
  
  #define BrocheLedGaucheTempo 3                                  // LED2 Bleue en série avec R12 valeur 33 ohms
  #define BrocheLedDroiteTempo 2                                  // LED1 Jaune en série avec R11 valeur 82 ohms             
  
  #define MosfetGate 5
  bool Outtime = false;
  
  #define BPMosfetGate 7                                          // 1 (vert) fil blanc
  int etatBPMosfetGate = LOW;

  #define BPPause 8                                               // 2 (jaune) fil jaune
  int etatBPPause = LOW;

  #define BPStart 9                                               // 3 (jaune) fil noir
  int etatBPStart = LOW;

  
  
  #define BPAux 10                                                // 4 (jaune) fil vert
  int etatBPAux = LOW;

    
  #define LED_PIN1    6
               
  #define NUM_LEDS1 100                                           // Nombre total de LEDs WS2812B utilisées (4 matrices de 5X5)                          

  Adafruit_NeoPixel strip1(NUM_LEDS1, LED_PIN1, NEO_GRB + NEO_KHZ800);
   
  float a;  

  byte colors[3][3] = { {0xff, 0,0},
                        {0xff, 0xff, 0xff},
                        {0   , 0   , 0xff} };

  // RunningLights(0xff,0,0, 50);        // red
  // RunningLights(0xff,0xff,0xff, 50);  // white
  //RunningLights(0,0,0xff, 50);        // blue

  unsigned long interval;                                         // Initialisation variable permettant de gérer la tempo de mise à l'arrêt des auxilliaires

  unsigned long zz = 0;                                           // Initialisation variable permettant de sortir de la fonction BouncingColoredBalls()
   
  void setup() 
  {
  analogWrite(BrocheDAC, 1000);
  
  int ValueDAC = 0;
  
  pinMode (MosfetGate, OUTPUT);

  pinMode (BrocheBuzzer, OUTPUT);

  pinMode (BPMosfetGate, INPUT_PULLUP);
  
  pinMode (BPPause, INPUT_PULLUP);

  pinMode (BPStart, INPUT_PULLUP);

  pinMode (BPAux, INPUT_PULLUP);
  
  pinMode (BrocheLedGaucheTempo, OUTPUT);

  pinMode (BrocheLedDroiteTempo, OUTPUT);
   
  analogReadResolution(10);                                        // Spécification résolution à utiliser pour la lecture ANA
  analogWriteResolution(10);                                       // Spécification résolution à utiliser pour l'écriture ANA (DAC)

  digitalWrite (BrocheLedGaucheTempo, LOW);                        // Allumage LED bleue (bord droit zone1)
  digitalWrite (BrocheLedDroiteTempo, LOW);                        // Allumage LED jaune (bord gauche zone3) nota : si nous sommes en zone2 LED bleue et jaune allumées
  
  
  digitalWrite (MosfetGate, HIGH);
  
  //|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  delay(300);                                                      //||||||||||||||| Attention tempo très importante sinon le DFPlayer mini ne démarre pas ||||||||||||||||||
  //|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  
  Buzzer(50, 120, 5);
  Serial.begin(115200);                                            // Démarrage de la liaison série (pour moniteur et traceur série)

  strip1.begin();           
  strip1.show();            
  strip1.setBrightness(1); 
   
  delay (10);
  }

  void loop() 
  {
  analogWrite(BrocheDAC, ValueDAC);                                // Positionnement de la dernière ValueDAC sur son port
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  analogRead (potPinInterval);                                         
  int ab = analogRead (potPinInterval);
  
  //*********************************** 

  //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  //||||||||||||||||||||||||| IMPORTANT : CI-DESSOUS TOUS LES CALCULS SONT EFFECTUES EN MILLISECONDES, J'AURAIS PU LES CONVERTIR DES LE DEBUT EN MINUTES |||||||||||||||||||||||
  //|||||||||||||||||||||||||||||||||||||||| POUR TRAVAILLER AVEC DES NOMBRES MOINS LONGS, C'EST CE QUE JE FERAI LA PROCHAINE FOIS ||||||||||||||||||||||||||||||||||||||||||||]
  //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  
  delay (5);
  if (ab < 342) 
  {
  interval = 10*( map (ab, 0, 341, 0,36000));                      // Segment1 de 0 à 6 mn sur le 1er tiers du potentiomètre,le facteur 10 est pour diminuer les nombres dans le map
  digitalWrite (BrocheLedGaucheTempo, HIGH);
  digitalWrite (BrocheLedDroiteTempo, LOW);
  }
  delay (5);
   
  if ( (ab < 683) && (ab > 341))
  {
  interval = 10*( map (ab, 342,682,36000,360000));                 // Segment2 de 6 à 60 mn sur le 2ème tiers du potentiomètre
  digitalWrite (BrocheLedGaucheTempo, HIGH);
  digitalWrite (BrocheLedDroiteTempo, HIGH);
  
  }
  delay (5);

  if ( (ab < 1024) && (ab > 682))
  {
  interval = 10*( map (ab, 683,1023,360000,3600000));              // Segment3 de 60 à 600 mn sur le 3ème tiers du potentiomètre
  digitalWrite (BrocheLedGaucheTempo, LOW);
  digitalWrite (BrocheLedDroiteTempo, HIGH);
  }
  delay (5);

  Serial.println ((interval/60000) + 1);                           // Affichage de la tempo en minutes (en rajoutant une minute "parasite")
  
  if (millis() >= interval && Outtime == false)                    // Outime false permet de n'actionner la tempo qu'une fois parmi tous les cycles
     
  {
  Buzzer(130, 200, 8);
  ValueDAC = 310;
  delay (10);
  analogWrite(BrocheDAC, ValueDAC);
  delay (100);
  digitalWrite (MosfetGate, LOW);
  etatBPMosfetGate = LOW;
  delay (10);
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  strip1.clear();
  TestBP ();
  strip1.show();
  delay (10);

  Outtime =true;
  }
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  delay(10);
  strip1.clear();
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  byte colors[3][3] = { {0xff, 0,0},
                        {0xff, 0xff, 0xff},
                        {0   , 0   , 0xff} };


//########################################################## Liste des scenarri lumineux à enchaîner DEBUT #############################################
 
  TwinkleRandom(800, 1, true);                              
  TwinkleRandom(800, 1, false);                             

  
  for (int jjj=0; jjj <10; jjj++)
    {
    for (int jj=0; jj <50; jj++)
    {
    Sparkle(random(255), random(255), random(255), 0);
    }
  
    for (int jj=0; jj <10; jj++)
    {
    Sparkle(random(255), random(255), random(255), 100);
    } 
  }
  
  for (int jj=0; jj <700; jj++)
  {
  Sparkle(random(255), random(255), random(255), 0);
  }
  
  for (int jj=0; jj <100; jj++)
  {
  Sparkle(random(255), random(255), random(255), 100);
  }

  BouncingColoredBalls(3, colors);                                 
   
  rainbow(.0001);
  rainbow(.001);
    
  theaterChaseRainbow(4);                                           
  
  rainbowCycle(20);
    
  FadeInOut(0x00, 0x00, 0xff); // blue                              
  
  RunningLights(0,0,0xff, 50);        // blue                           
  
  NewKITT(0xff, 0, 0, 8, 10, 50);
  
  SnowSparkle(0x10, 0x10, 0x10, 20, 200);      
  
  CylonBounce(0xff, 0, 0, 4, 10, 50);                                               
  
  CylonBounce(0xff, 0, 0, 20, 1, 1); 
  CylonBounce(0, 0xff, 0, 20, 1, 1);                                                
  CylonBounce(0, 0, 0xff, 20, 1, 1);                                                
  
  //########################################################## Liste des scenarri lumineux à enchainer FIN #############################################
  
  }

   //*****************************************************************************************************************************************
  
  void showStrip() 
  {
  #ifdef ADAFRUIT_NEOPIXEL_H 
  // NeoPixel
  strip1.show();
  #endif
  #ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  FastLED.show();
  #endif
  }

  void setPixel(int Pixel, byte red, byte green, byte blue) 
  {
  #ifdef ADAFRUIT_NEOPIXEL_H 
  // NeoPixel
  strip1.setPixelColor(Pixel, strip1.Color(red, green, blue));
  #endif
  #ifndef ADAFRUIT_NEOPIXEL_H 
  // FastLED
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
  #endif
  }

  void setAll(byte red, byte green, byte blue) 
  {
  for(int i = 0; i < NUM_LEDS1; i++ ) 
  {
  setPixel(i, red, green, blue);
  }
  showStrip();
  }
  
  //*****************************************************************************************************************************************


  void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) 
  {  
  setAll(0,0,0);
 
  for(int i = 0; i < NUM_LEDS1+NUM_LEDS1; i++) 
  {
  for(int j=0; j<NUM_LEDS1; j++) 
  {
  
  if( (!meteorRandomDecay) || (random(10)>5) ) 
  {
  fadeToBlack(j, meteorTrailDecay );        
  }
  }
  
  for(int j = 0; j < meteorSize; j++) 
  {
  
  if( ( i-j <NUM_LEDS1) && (i-j>=0) ) 
  {
  setPixel(i-j, red, green, blue);
  }
  
  }
  showStrip();
  delay(SpeedDelay);
  }
  }

  void theaterChaseRainbow(int SpeedDelay) 
  {
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  byte *c;
  
  TestBP ();
  
  for (int j=0; j < 256; j++) 
  {     // cycle all 256 colors in the wheel
  for (int q=0; q < 3; q++) 
  {
  for (int i=0; i < NUM_LEDS1; i=i+3) 
  {
  c = Wheel( (i+j) % 255);
  setPixel(i+q, *c, *(c+1), *(c+2));    //turn every third pixel on
  
  //TestBP ();                                                     // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  }
  
  TestBP ();
  
  showStrip();
  delay(SpeedDelay);
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  for (int i=0; i < NUM_LEDS1; i=i+3) 
  {
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  setPixel(i+q, 0,0,0);        //turn every third pixel off
  }
  }
  }
  }

  void fadeToBlack(int ledNo, byte fadeValue) 
  {
  #ifdef ADAFRUIT_NEOPIXEL_H
  uint32_t oldColor;
  uint8_t r, g, b;
  int value;
     
  oldColor = strip1.getPixelColor(ledNo);
  r = (oldColor & 0x00ff0000UL) >> 16;
  g = (oldColor & 0x0000ff00UL) >> 8;
  b = (oldColor & 0x000000ffUL);

  r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
  g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
  b=(b<=10)? 0 : (int) b-(b*fadeValue/256);
   
  strip1.setPixelColor(ledNo, r,g,b);
  #endif
  #ifndef ADAFRUIT_NEOPIXEL_H
  #endif  
  }

  void RunningLights(byte red, byte green, byte blue, int WaveDelay) 
  {
  int Position=0;
  for(int j=0; j<NUM_LEDS1*2; j++)
  {
  Position++; // = 0; //Position + Rate;
  for(int i=0; i<NUM_LEDS1; i++) 
  {
  // sine wave, 3 offset waves make a rainbow!
  //float level = sin(i+Position) * 127 + 128;
  //setPixel(i,level,0,0);
  //float level = sin(i+Position) * 127 + 128;
  setPixel(i,((sin(i+Position) * 127 + 128)/255)*red,
  ((sin(i+Position) * 127 + 128)/255)*green,
  ((sin(i+Position) * 127 + 128)/255)*blue);
  }
     
  showStrip();
  delay(WaveDelay);
  }
  }

  void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) 
  {
  setAll(red,green,blue);
  int Pixel = random(NUM_LEDS1);
  setPixel(Pixel,0xff,0xff,0xff);
  
  showStrip();
  delay(SparkleDelay);
  setPixel(Pixel,red,green,blue);
  showStrip();
  delay(SpeedDelay);
  }
  
  void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
  {
  for(int i = 0; i < NUM_LEDS1-EyeSize-2; i++) 
  {
  setAll(0,0,0);
  TestBP ();
  setPixel(i, red/10, green/10, blue/10);
  for(int j = 1; j <= EyeSize; j++) 
  {
  setPixel(i+j, red, green, blue);
  }
  setPixel(i+EyeSize+1, red/10, green/10, blue/10);
  showStrip();
  delay(SpeedDelay);
  }
  delay(ReturnDelay);
  for(int i = NUM_LEDS1-EyeSize-2; i > 0; i--) 
  {
  setAll(0,0,0);
  setPixel(i, red/10, green/10, blue/10);
  for(int j = 1; j <= EyeSize; j++) 
  {
  setPixel(i+j, red, green, blue);
  }
  setPixel(i+EyeSize+1, red/10, green/10, blue/10);
  showStrip();
  delay(SpeedDelay);
  }
  delay(ReturnDelay);
  }

  void NewKITT(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
  {
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  LeftToRight(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  RightToLeft(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  OutsideToCenter(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  CenterToOutside(red, green, blue, EyeSize, SpeedDelay, ReturnDelay);
  }

  void CenterToOutside(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) 
  {
  for(int i =((NUM_LEDS1-EyeSize)/2); i>=0; i--) 
  {
  setAll(0,0,0);
  setPixel(i, red/10, green/10, blue/10);
  for(int j = 1; j <= EyeSize; j++) 
  {
  setPixel(i+j, red, green, blue);
  }
  setPixel(i+EyeSize+1, red/10, green/10, blue/10);
  setPixel(NUM_LEDS1-i, red/10, green/10, blue/10);
  for(int j = 1; j <= EyeSize; j++) 
  {
  setPixel(NUM_LEDS1-i-j, red, green, blue);
  }
  setPixel(NUM_LEDS1-i-EyeSize-1, red/10, green/10, blue/10);
  showStrip();
  delay(SpeedDelay);
  }
  delay(ReturnDelay);
  }

  void OutsideToCenter(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) 
  {
  for(int i = 0; i<=((NUM_LEDS1-EyeSize)/2); i++) 
  {
  setAll(0,0,0);
  setPixel(i, red/10, green/10, blue/10);
  for(int j = 1; j <= EyeSize; j++) 
  {
  setPixel(i+j, red, green, blue);
  }
  setPixel(i+EyeSize+1, red/10, green/10, blue/10);
  setPixel(NUM_LEDS1-i, red/10, green/10, blue/10);
  for(int j = 1; j <= EyeSize; j++) 
  {
  setPixel(NUM_LEDS1-i-j, red, green, blue);
  }
  setPixel(NUM_LEDS1-i-EyeSize-1, red/10, green/10, blue/10);
  showStrip();
  delay(SpeedDelay);
  }
  delay(ReturnDelay);
  }

  void LeftToRight(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) 
  {
  for(int i = 0; i < NUM_LEDS1-EyeSize-2; i++) 
  {
  setAll(0,0,0);
  setPixel(i, red/10, green/10, blue/10);
  for(int j = 1; j <= EyeSize; j++) 
  {
  setPixel(i+j, red, green, blue);
  }
  setPixel(i+EyeSize+1, red/10, green/10, blue/10);
  showStrip();
  delay(SpeedDelay);
  }
  delay(ReturnDelay);
  }

  void RightToLeft(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay) 
  {
  for(int i = NUM_LEDS1-EyeSize-2; i > 0; i--) 
  {
  setAll(0,0,0);
  setPixel(i, red/10, green/10, blue/10);
  for(int j = 1; j <= EyeSize; j++) 
  {
  setPixel(i+j, red, green, blue);
  }
  setPixel(i+EyeSize+1, red/10, green/10, blue/10);
  showStrip();
  delay(SpeedDelay);
  }
  delay(ReturnDelay);
  }

  void Strobe(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause)
  {
  for(int j = 0; j < StrobeCount; j++) 
  {
  setAll(red,green,blue);
  
  TestBP();                                                        // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  showStrip();
  delay(FlashDelay);
  setAll(0,0,0);
  
  TestBP();                                                        // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  showStrip();
  delay(FlashDelay);
  }
  delay(EndPause);
  }

  void FadeInOut(byte red, byte green, byte blue)
  {
  float r, g, b;
  for(int k = 0; k < 256; k=k+1) 
  {
  r = (k/256.0)*red;
  g = (k/256.0)*green;
  b = (k/256.0)*blue;
  setAll(r,g,b);
  showStrip();
  }
  for(int k = 255; k >= 0; k=k-2) 
  {
  r = (k/256.0)*red;
  g = (k/256.0)*green;
  b = (k/256.0)*blue;
  setAll(r,g,b);
  showStrip();
  }
  }

  void colorWipe(byte red, byte green, byte blue, int SpeedDelay) 
  {
  for(uint16_t i=0; i<NUM_LEDS1; i++) 
  {
  setPixel(i, red, green, blue);
  showStrip();
  delay(SpeedDelay);
  }
  }

  //**********************************************************************
  //**********************************************************************
  void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) 
  {
  setAll(0,0,0);
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  for (int i=0; i<Count; i++) 
  {
  setPixel(random(NUM_LEDS1),random(0,255),random(0,255),random(0,255));
  TestBP ();
  showStrip();
     
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
     
  delay(SpeedDelay);  
  
  if(OnlyOne) 
  {
       
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
       
  setAll(0,0,0);
  }
  
  }
  delay(SpeedDelay);
  }
  //**********************************************************************
  //**********************************************************************

  void RGBLoop()
  {
  for(int j = 0; j < 3; j++ ) 
  {
  
  // Fade IN
    
  for(int k = 0; k < 256; k++) 
  {
  switch(j) 
  {
  case 0: setAll(k,0,0); break;
  case 1: setAll(0,k,0); break;
  case 2: setAll(0,0,k); break;
  }
  showStrip();
  delay(3);
  }
    
  // Fade OUT
    
  for(int k = 255; k >= 0; k--) 
  {
  switch(j) {
  case 0: setAll(k,0,0); break;
  case 1: setAll(0,k,0); break;
  case 2: setAll(0,0,k); break;
  }
  showStrip();
  delay(3);
  }
  }
  }

  
  
  void BouncingColoredBalls(int BallCount, byte colors[][3]) 
    
  {
  float Gravity = -9.81;
  int StartHeight = 1;
  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * StartHeight );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];
 
  for (int i = 0 ; i < BallCount ; i++)
  {  
 
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
    
  ClockTimeSinceLastBounce[i] = millis();
  Height[i] = StartHeight;
  Position[i] = 0;
  ImpactVelocity[i] = ImpactVelocityStart;
  TimeSinceLastBounce[i] = 0;
  Dampening[i] = 0.90 - float(i)/pow(BallCount,2);
  }

  while (true) 
 
  {
  for (int i = 0 ; i < BallCount ; i++) 
  {
  TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
  Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i]/1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i]/1000;
  
  if ( Height[i] < 0 ) 
  {                      
  Height[i] = 0;
  ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
  ClockTimeSinceLastBounce[i] = millis();
  if ( ImpactVelocity[i] < 0.01 ) 
  {
  ImpactVelocity[i] = ImpactVelocityStart;
  }
  }
  
  Position[i] = round( Height[i] * (NUM_LEDS1 - 1) / StartHeight);
  }
  for (int i = 0 ; i < BallCount ; i++) 
  {
  setPixel(Position[i],colors[i][0],colors[i][1],colors[i][2]);
  }
  showStrip();
    
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
    
  setAll(0,0,0);

  zz = zz+1;

  if (zz > 10000)
  {
  zz = 0;
  return;
  }
  }
  }
  
  void rainbow(int wait) 
  {
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) 
  {
  for(int i=0; i<strip1.numPixels(); i++)
  { 
  int pixelHue = firstPixelHue + (i * 65536L / strip1.numPixels());
  strip1.setPixelColor(i, strip1.gamma32(strip1.ColorHSV(pixelHue)));
  }
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
   
  strip1.show();
  delay(wait);  
  }
  }

  void Sparkle(byte red, byte green, byte blue, int SpeedDelay) 
  {
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  int Pixel = random(NUM_LEDS1);
  setPixel(Pixel,red,green,blue);
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  showStrip();
  delay(SpeedDelay);
  setPixel(Pixel,0,0,0);
  }

  void rainbowCycle(int SpeedDelay) 
  {
  
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  
  byte *c;
  uint16_t i, j;
  for(j=0; j<256*5; j++) // 5 cycles of all colors on wheel
  { 
  for(i=0; i< NUM_LEDS1; i++) 
  {
  c=Wheel(((i * 256 / NUM_LEDS1) + j) & 255);
  setPixel(i, *c, *(c+1), *(c+2));
  }
    
  TestBP ();                                                       // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
    
  showStrip();
  delay(SpeedDelay);
  }
  }
  byte * Wheel(byte WheelPos) 
  {
  static byte c[3];
  
  if(WheelPos < 85) 
  {
  c[0]=WheelPos * 3;
  c[1]=255 - WheelPos * 3;
  c[2]=0;
  } 
  
  else if(WheelPos < 170) 
  {
  WheelPos -= 85;
  c[0]=255 - WheelPos * 3;
  c[1]=0;
  c[2]=WheelPos * 3;
  } 
  
  else 
  {
  WheelPos -= 170;
  c[0]=0;
  c[1]=WheelPos * 3;
  c[2]=255 - WheelPos * 3;
  }
  
  return c;
  }

  void TestBP ()                                                   // Renvoi vers la fonction TestBP () pour lecture des entrées associées et lancement actions associées
  {
  digitalRead (MosfetGate);
  digitalRead (BPMosfetGate);
  digitalRead (BPPause);
  digitalRead (BPStart);
  digitalRead (BPAux);
  analogRead (potPinInterval);
  int ab = analogRead (potPinInterval);                             // Le placement de cette action également à cet endroit permet d'avoir une acualisation quasi temps réel
  
  etatBPPause == LOW;
  etatBPStart == LOW;

  if (digitalRead (BPMosfetGate) == LOW && Outtime == false)
  
  {
  etatBPMosfetGate = HIGH;
  Buzzer(130, 200, 8);
  ValueDAC = 310;
  analogWrite(BrocheDAC, ValueDAC);
  delay (100);
  digitalWrite (MosfetGate, LOW);
  Outtime =true;
  delay (10);
  }
      
  if (digitalRead (BPPause) == LOW && etatBPPause == HIGH)
  {
  Buzzer(600, 60, 1);
  delay (10);
  etatBPPause = HIGH;
  delay (10);
  }
    
  if (digitalRead (BPPause) == LOW && etatBPPause == LOW )
  {
   Buzzer(70, 70, 2);
  delay (10);
  etatBPPause = HIGH;
  delay (10);
  etatBPStart = LOW;
  }
  
  if (digitalRead (BPStart) == LOW && etatBPStart == HIGH )
  {
  Buzzer(600, 60, 1);
  delay (10);
  etatBPStart = HIGH;
  delay (10);
  }
      
  if (digitalRead (BPStart) == LOW && etatBPStart == LOW )
  {
  Buzzer(70, 70, 2);
  delay (10);
  etatBPStart = HIGH;
  delay (10);
  etatBPPause = LOW;
  }
            
  if (digitalRead (BPAux) == LOW)
  {
  etatBPAux = HIGH;
  }
                         
  if (etatBPPause == HIGH)
  {
  etatBPStart = LOW;
  ValueDAC = 310;
  analogWrite(BrocheDAC, ValueDAC);
  }
    
  if (etatBPStart == HIGH)
  {
  etatBPPause = LOW;
  ValueDAC = 620;
  analogWrite(BrocheDAC, ValueDAC);
  }
  
   if (etatBPAux == HIGH)
  {
  NVIC_SystemReset();
  etatBPAux = LOW;
  }
  
  delay (5);
  if (ab < 342) 
  {
  interval = 10*( map (ab, 0, 341, 0,36000));                      //Segment1 de 0 à 6 mn sur le 1er tiers du potentiomètre
  digitalWrite (BrocheLedGaucheTempo, HIGH);
  digitalWrite (BrocheLedDroiteTempo, LOW);
  }
  //delay (5);
   
  if ( (ab < 683) && (ab > 341))
  {
  interval = 10*( map (ab, 342,682,36000,360000));                 //Segment2 de 6 à 60 mn sur le 2ème tiers du potentiomètre
  digitalWrite (BrocheLedGaucheTempo, HIGH);
  digitalWrite (BrocheLedDroiteTempo, HIGH);
  
  }
  //delay (5);
  analogRead (potPinInterval);                                         
  
  if ( (ab < 1024) && (ab > 682))
  {
  interval = 10*( map (ab, 683,1023,360000,3600000));              //Segment3 de 60 à 600 mn sur le 3ème tiers du potentiomètre
  digitalWrite (BrocheLedGaucheTempo, LOW);
  digitalWrite (BrocheLedDroiteTempo, HIGH);
  }
  //delay (5);
  
  }

  void Buzzer (int TempsH, int TempsL, int nb)                      // TempsH => délai buzzer ON, TempsL => délai buzzer OFF, nb => nombre de bips
  {
  for (int x = 1; x <= nb; x++)                                     // Boucle le nombre de fois voulu passé par l'argument "int nb"
  {
  digitalWrite(BrocheBuzzer, HIGH);                                 // Active le buzzer
  delay (TempsH);                                                   // Temporisation à l'état haut du buzzer pendant la durée passée par l'argument "int TempsH"
  digitalWrite(BrocheBuzzer, LOW);                                  // Désactive le buzzer
  delay (TempsL);                                                   // Temporisation à l'état bas du buzzer pendant la durée passée par l'argument "int TempsL"
  }
  }

 
   
