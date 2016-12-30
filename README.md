# BinarySwitchMinimalMySensors
MySensors BinarySwitchMinimalMySensors Battery Powered Door/Window sensor

This is a binary switch sketch for MySensors 2.0.0. Based on the BinarySwitch example, changed to work with a sleep timeout.

At first, the sleep command was released for an interrupt very fast. This was due to the fact there was no pullup on D2 and D3. Changed the setup() code to 

    // Setup the buttons
    pinMode(PRIMARY_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SECONDARY_BUTTON_PIN, INPUT_PULLUP);

Then I wanted to not get only messages when an interrupt was fired but also periodically for every 10 minutes or so. At first I used an int var to set a sleeptime for 60*1000*10 ms. But the sleep was released every some seconds. Then I realized, that the sleep does not return a boolean but an int8_t. Where -1 represents the return value for a timeout, and 0 and 1 are returned for the first and second interrupt pin assignment. For further processing I used a switch function:

	  int8_t mysleep=sleep(SECONDARY_BUTTON_PIN-2, CHANGE, sleeptime);
	  //sleep returns -1 for timeout, or the PIN INT (first or second INT argument) that fired the interrupt!
	  switch (mysleep){
		case -1:    
		  send(msg2.set(value==HIGH ? 1 : 0), NEED_ACK);
		  sendBatteryLevel(getBatteryLevel(), NEED_ACK);
		  #ifdef MY_DEBUG
		  Serial.println("######## wake up by time out");
		  #endif
		  break;
		case PRIMARY_BUTTON_PIN-2:
		  //msg will be send above due to change
		  #ifdef MY_DEBUG
		  Serial.println("######## wake up PRIMARY_BUTTON_PIN change");
		  #endif
		  break;
		case SECONDARY_BUTTON_PIN-2:
		  //msg will be send above due to change
		  #ifdef MY_DEBUG
		  Serial.println("######## wake up SECONDARY_BUTTON_PIN change");
		  #endif
		  break;
		default:
		  #ifdef MY_DEBUG
		  Serial.print("######## sleep returned: ");Serial.println(mysleep);
		  #endif
		  break;
	  }

With that I verified that the sleep return was casued by the timout. But the interval was by far incorrect. The sleep timeout argument is specified as an unsigned long. So, I tried casting my int sleeptime to unsigned long. That did not fix the weired timeouts. I then added a debug output to see what value is used as sleeptime and did not get my 600000 but very different number. Researching for a solution I stumbled about the number specifier UL. I changed the sleeptime numbering to:

	  //define sleeptime as unsigned long and use UL specifier! or you get weird numbers :-((
	  unsigned long sleeptime = 600000UL; // 10 minutes are 1000*60*10
	  #ifdef MY_DEBUG
	  Serial.print("######## sleeptime: ");Serial.println(sleeptime);
	  #endif

and the 300000UL did the trick. The sketch now wakes up by either the interrupt PIN or the specified timeout.

The Arduino compiler did too much code and memory optimization and stored the 600000 in a smaller memory area than needed by an unsigned long. If that value is supplied to a function that looks for an unsigned long, the value will be interpreted differently and gives the wrong number!
