var setPebbleToken = 'YWFN';

function appMessageAck(e) {
    console.log("Options sent to Pebble successfully");
	//Pebble.showSimpleNotificationOnPebble("YES", "Message to pebble sent");
}

function appMessageNack(e) {
    console.log("Failed to send options to Pebble: " + e.error.message);
	//Pebble.showSimpleNotificationOnPebble("NO", "Unable to send message to pebble" + e.error.message);
}

Pebble.addEventListener('ready', function(e) {
	console.log("Ready listener fired");
	//Pebble.showSimpleNotificationOnPebble("Pokus", "toto je notifikacia ze JS je ready");
	//TODO - send configuration to watch
});

Pebble.addEventListener('showConfiguration', function(e) {
	var url = 'http://x.SetPebble.com/' + setPebbleToken + '/' + Pebble.getAccountToken();
	//Pebble.showSimpleNotificationOnPebble("Opening config page", url);
	console.log("Showing config page: " + url);
	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(reply) {
	console.log("configuration closed");
	//Pebble.showSimpleNotificationOnPebble("Config view closed", "...");
	
	if(reply.response.length>0){
		//var options = JSON.parse(decodeURIComponent(e.response));
		var options = JSON.parse(reply.response);
		//Pebble.showSimpleNotificationOnPebble("Sending options: ", JSON.stringify(options));	
		console.log("Sending options: " + JSON.stringify(options));
		localStorage.setItem(setPebbleToken, reply.response);
		Pebble.sendAppMessage(JSON.parse(reply.response), appMessageAck, appMessageNack);
		/*
		Pebble.sendAppMessage(JSON.parse(reply.response),
                        function(_event) {
							console.log("OK");
							//Pebble.showSimpleNotificationOnPebble("App send: ", "OK");
                        },
                        function(_event) {
                            console.log("App send Failed " + _event.error.message);
							//Pebble.showSimpleNotificationOnPebble("App send: ", "Failed");
                        }
        );
		*/
	} else {
		console.log("No options returned");
		//Pebble.showSimpleNotificationOnPebble("Options = ", "no options returned");	
	}
});

/*
Pebble.addEventListener('webviewclosed', function(e) {
	if ((typeof(e.response) == 'string') && (e.response.length > 0)) {
		console.log("Sending options to pebble");
		Pebble.sendAppMessage(JSON.parse(e.response), appMessageAck, appMessageNAck);
		console.log("Storing options locally");
		localStorage.setItem(setPebbleToken, e.response);
	} else{
		console.log("No options received");
		Pebble.showSimpleNotificationOnPebble("UPS", "No configuration received");
	}
	
});
*/
/*
Pebble.addEventListener('appmessage', function(e) {
  key = e.payload.action;
  if (typeof(key) != 'undefined') {
    var settings = localStorage.getItem(setPebbleToken);
    if (typeof(settings) == 'string') {
      try {
        Pebble.sendAppMessage(JSON.parse(settings));
      } catch (e) {
      }
    }
    var request = new XMLHttpRequest();
    request.open('GET', 'http://x.SetPebble.com/api/' + setPebbleToken + '/' + Pebble.getAccountToken(), true);
    request.onload = function(e) {
      if (request.readyState == 4)
        if (request.status == 200)
          try {
            Pebble.sendAppMessage(JSON.parse(request.responseText));
          } catch (e) {
          }
    }
    request.send(null);
  }
});
*/