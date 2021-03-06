  #include "Arduino.h"
  #include <SoftwareSerial.h>
  #include "DFRobotDFPlayerMini.h"
  #include <math.h>

  #define TX 0
  #define potPinSound A1
  #define ANADossier A2
  #define ANACommut A3

  int bb;
  int bbb;
  
  int etatBPPause = LOW;
  int etatBPStart = LOW;
 
  byte volumeLevel; //variable for holding volume level;
  float a;
  
  float ratio;                                                    // Pour log
  float ratio1024;                                                // Pour log
  
  // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
     
   SoftwareSerial mySoftwareSerial(1,0); // RX, TX
  
  // Réaffectation des RX/TX pour ne pas avoir de problème lors du téléversement
  // La résistance de 1K branchée en série sur le port n'a pas été installée, j'ai ajouté un adapteur de niveau du signal basé sur l'emploi d'une résistance en série avec une zener
  // d'émetteur et collecteur relié au 3,3V de la Nano
  // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  DFRobotDFPlayerMini myDFPlayer; 
  
  void setup()
  {
  pinMode (TX, OUTPUT);                                                     // Ouverture du port TX
  
  double y = 1025;                                                          // Pour log
  double LOG1025 = log10 (y);                                               // Pour log
 
  a= analogRead(potPinSound);                                               // Pour log
  double x = a;                                                             // Pour log
  double LOG10x = log10 (x);
  ratio = log10 (x) / log10 (y);                                            // Pour log
  ratio1024 = ratio*1024;                                                   // Pour log
    
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // ===================================== La première ligne de commande sert à déterminer le voulume linéairement : ======================================
  //
  // volumeLevel = map(analogRead(potPinSound), 0, 1023, 0, 30);
  //
  // ====================== La deuxième ligne de commande sert à déterminer le voulume avec correction logarithmique logicielle : =========================
     volumeLevel = map(ratio1024, 0, 1023, 0, 30);
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  mySoftwareSerial.begin(9600);
  myDFPlayer.begin (mySoftwareSerial);
  pinMode (TX, OUTPUT); 
  
  // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++On sélectionne une piste et on la fait tourner en boucle++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  //-----------------------------------------------------------------------Sélection du dossier MP3----------------------------------------------------------
  myDFPlayer.volume(volumeLevel);
  
  bbb = analogRead (ANADossier);
  analogRead (ANADossier);                                                // Valeur ANA en provenance XIAO pour sélectionner un des trois dossiers
  bbb = analogRead (ANADossier);
  delay (10); 
      
  if ( bbb < 341)                                                         // Chants de Noel
  {myDFPlayer.loopFolder(01);}                                            // loop all mp3 files in folder SD:/01
  delay (10);
                                  
  if (  bbb > 340 &&  bbb < 628)                                          // Comptines
  {myDFPlayer.loopFolder(02);}                                            //loop all mp3 files in folder SD:/02
  delay (10);  
                                 
  if (  bbb > 627 )                                                       // Chants d'oiseaux
  {myDFPlayer.loopFolder(03);}                                            // loop all mp3 files in folder SD:/03
  delay (10);
   
  //myDFPlayer.randomAll();
  delay (2000);                                                           // Tempo pour nécessaire à l'éxécution de la sélection de la piste
  pinMode (TX, INPUT_PULLUP);                                             // Une fois la commande DFPlayer passée, le port TX est positionné en haute impédance
  // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 
  }
 
  void loop()
  {
  volumeLevel = 0;
  TestANACommut ();
  }

  void TestANACommut ()
  {
  bb = analogRead (ANACommut); 
  analogRead (ANACommut);
  //delay (100);
  delay (5);
    
  etatBPPause == LOW;
  etatBPStart == LOW;   

  if (  (bb > 130) &&  (bb < 280) && etatBPPause == LOW)
  {
  delay (2);
  etatBPPause = HIGH;
  delay (2);
  etatBPStart = LOW;
  }
   
  if (  (bb > 320) &&  (bb < 500) && (etatBPStart == LOW))
  {
  delay (2);
  etatBPStart = HIGH;
  //delay (10);
  delay (2);
  etatBPPause = LOW;
  }
                          
  if (etatBPPause == HIGH)
  {
  etatBPStart = LOW;
  pinMode (TX, OUTPUT);
  delay (10);
  myDFPlayer.pause ();
  delay (600);
  pinMode (TX, INPUT_PULLUP);
  delay (10);
  }

  if (etatBPStart == HIGH)
  {
  etatBPPause = LOW;
  pinMode (TX, OUTPUT);
  delay (10);
  myDFPlayer.start ();
  delay (600);
  pinMode (TX, INPUT_PULLUP);
  delay (10);
  }
  }
  
  
 
