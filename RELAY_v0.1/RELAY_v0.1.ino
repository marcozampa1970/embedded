const short int BUILTIN_LED1 = 2; //GPIO2
const short int BUILTIN_LED2 = 16; //GPIO16



void setup() {

  pinMode(BUILTIN_LED1, OUTPUT); // Initialize the BUILTIN_LED1 pin as an output
  pinMode(BUILTIN_LED2, OUTPUT); // Initialize the BUILTIN_LED2 pin as an output

  pinMode(D2, OUTPUT); // Initialize the BUILTIN_LED2 pin as an output

}

void loop() {

  digitalWrite(BUILTIN_LED1, LOW);
  digitalWrite(BUILTIN_LED2, HIGH);
  digitalWrite(D2, HIGH);
  delay(15000);

  digitalWrite(BUILTIN_LED1, HIGH);
  digitalWrite(BUILTIN_LED2, LOW);
  digitalWrite(D2, LOW);  
  delay(15000);


}
