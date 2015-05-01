/**
 * Nummernschalter-Prüfprogramm.
 * Prüft einen angeschlossenen Nummernschalter und gibt das Ergebnis per serieller Schnittstelle aus.
 * 4 / 2015
 */

// Zeiteinstellungen
#define lowerThresholdImpulse 55 // Untere Grenze für einen Impuls, in Millisekunden (Optimal: 62 ms)
#define upperThresholdImpulse 72 // Obere Grenze für einen Impuls, in Millisekunden
#define lowerThresholdPause   31 // Untere Grenze für eine Pause, in Millisekunden (Optimal: 38 ms)
#define upperThresholdPause   48 // Obere Grenze für eine Pause, in Millisekunden
#define debounce 2 // Debounce-Zeit, in Millisekunden (Könnte für schlechte Nummernschalter nach oben korrigiert werden müssen)

// Pineinstellungen
#define led 13
#define nsa 2 // Pin des nsa-Kontaktes
#define nsi 3 // Pin des nsi-Kontaktes
#define baudr 115200 // Baud-Rate für Serielle Verbindung
#define inputPullups true

uint8_t nsiImpulses = 0;
uint8_t invalidImpulses = 0;
uint8_t invalidPauses = 0;
uint16_t minMaxImpulse[2] = {-1,0};
uint16_t minMaxPause[2] = {-1,0};

unsigned long nsiImpulseTime = -1;
unsigned long nsiPauseTime = -1;
unsigned long nsaTime = -1;

bool nsaOldState = inputPullups; //NO
bool nsiOldState = !inputPullups; //NC

void setup() {
	pinMode(led, OUTPUT);
#if inputPullups == false // Externe Pull-Down-Widerstände
	pinMode(nsa, INPUT);
	pinMode(nsi, INPUT);
	#define nsaRead() digitalRead(nsa)
	#define nsiRead() digitalRead(nsi)
#else // Interne Pullups. Bisher ungetestet.
	pinMode(nsa, INPUT_PULLUP);
	pinMode(nsi, INPUT_PULLUP);
	#define nsaRead() !digitalRead(nsa)
	#define nsiRead() !digitalRead(nsi)
#endif
	Serial.begin(baudr);
	delay(20);
	Serial.print(F("Nummernschalter-Pruefprogramm.\nnsi-Kontakt an Pin "));
	Serial.print(nsi);
	Serial.print(F(" anklemmen, nsa-Kontakt an Pin "));
	Serial.print(nsa);
	Serial.print(F(" anklemmen.\n"));
}

void loop() {
	if(nsaRead() != nsaOldState) {
		delay(debounce); //Debouncing
		if(nsaRead() != nsaOldState) {
			nsaOldState = !nsaOldState;
			nsaChange();
		}
	}
	if(nsiRead() != nsiOldState) {
		delay(debounce); //Debouncing
		if(nsiRead() != nsiOldState) {
			nsiOldState = !nsiOldState;
			nsiChange();
		}
	}
}

void nsaChange() {
	if(nsaRead()) {
		// Wählvorgang beginnt
		nsiImpulses = 0;
		invalidImpulses = 0;
		invalidPauses = 0;
		nsiImpulseTime = millis();
		nsiPauseTime = millis();
		nsaTime = millis();

		// Serielle Ausgabe
		Serial.print(F("nsa geschlossen, Waehlvorgang beginnt\n"));
		// Statistik zurücksetzen
		minMaxImpulse[0] = -1;
		minMaxImpulse[1] =  0;
		minMaxPause[0]   = -1;
		minMaxPause[1]   =  0;
	} else {
		// Wählvorgang beendet
		Serial.print(F("nsa geoeffnet, Waehlvorgang beendet\n"));
		Serial.print("Impulse (gesamt/gueltig/ungueltig): (");
		Serial.print(nsiImpulses+invalidImpulses);
		Serial.print("/");
		Serial.print(nsiImpulses);
		Serial.print("/");
		Serial.print(invalidImpulses);
		Serial.print("), nsa Oeffnungszeit: ");
		Serial.print(millis() - nsaTime);
		Serial.print(" ms\n");
		if(nsiImpulses > 1) {
			// Bei einem oder weniger Impulsen macht das keinen Sinn.
			Serial.print("Impulsdauer (Min/Max): ");
			Serial.print(minMaxImpulse[0]);
			Serial.print("/");
			Serial.print(minMaxImpulse[1]);
			Serial.print(", Pausendauer (Min/Max): ");
			Serial.print(minMaxPause[0]);
			Serial.print("/");
			Serial.print(minMaxPause[1]);
		}
		Serial.print("\n");

	}
}

void nsiChange() {
	unsigned long diff = -1;
	if(!nsiRead()) {
		// Impuls Start
		diff = millis() - nsiPauseTime; // Impulslänge berechnen; Debounce-Zeit wird nicht berücksichtigt, da bei steigender und fallender Flanke debounced wird
		if(nsiImpulses != 0) {
			//MinMax Funktion
			if(diff < minMaxPause[0]) {
				minMaxPause[0] = diff;
			}
			if(diff > minMaxPause[1]) {
				minMaxPause[1] = diff;
			}
		}
		if(nsiImpulses == 0 || (diff >= lowerThresholdPause && diff <= upperThresholdPause)) {
			// Gültige Pause
			nsiImpulseTime = millis();
			Serial.print(F("\tGueltige Pause. Laenge:\t\t"));
			Serial.print(diff);
			Serial.print(" ms\n");
		} else {
			// Ungültige Pause
			nsiImpulseTime = millis();
			invalidPauses++;
			Serial.print(F("\tUngueltige Pause. Laenge:\t"));
			Serial.print(diff);
			Serial.print(" ms\n");
		}
	} else {
		// Pause Start
		diff = millis() - nsiImpulseTime;
		if(diff < minMaxImpulse[0]) {
			minMaxImpulse[0] = diff;
		}
		if(diff > minMaxImpulse[1]) {
			minMaxImpulse[1] = diff;
		}
		if(diff >= lowerThresholdImpulse && diff <= upperThresholdImpulse) {
			// Gültiger Impuls
			nsiPauseTime = millis();
			nsiImpulses++;
			Serial.print(F("\tGueltiger Impuls. Laenge:\t"));
			Serial.print(diff);
			Serial.print(" ms\n");
		} else {
			nsiPauseTime = millis();
			invalidImpulses++;
			Serial.print(F("\tUngueltiger Impuls. Laenge:\t"));
			Serial.print(diff);
			Serial.print(" ms\n");
		}
	}
}
