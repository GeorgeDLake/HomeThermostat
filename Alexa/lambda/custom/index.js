var Alexa = require('alexa-sdk');
var http = require('http');
var tempSlot = '';


//-----Handeler Section-----
exports.handler = function(event, context, callback){
  var alexa = Alexa.handler(event, context);
  alexa.registerHandlers(handlers);
  alexa.execute();
};

var handlers = 
{
  'LaunchRequest': function () 
  {
    
/*
	var speech = new Speech();
	speech.say('This is a test response & works great!');
	speech.pause('100ms');
	speech.say('How can I help you?');    
	speech.phoneme('ipa', "ɛl̪ tɛɾ.mos.ˈta.to se ˈa ɛ̃n.sɛ̃n̪.ˈdi.ðo", 'el termostato se ha encendido');
	var speechOutput = speech.ssml(true);        
	this.emit(':ask', speechOutput); 	
*/	
	//You say, <phoneme alphabet="ipa" ph="pɪˈkɑːn">pecan</phoneme>. 
	
    //var speechOutput = 'You say, <phoneme alphabet="ipa" ph="pɪˈkɑːn">pecan</phoneme>. I say, <phoneme alphabet="ipa" ph="ˈpi.kæn">pecan</phoneme>.';
	this.emit(':ask','Thermostat is ready for your request');
  },
  
  'Unhandeld': function()
  {
	  var movieSlot = intent.slots.Movie.value;
	  this.emit(':tell','I have no clue what to do');
  },
  'SpanishTest': function(){
		SpanishTest((data) =>{
		var outputSpeech = data
		this.emit(':tell', outputSpeech);
		});
	},  
  'TurnOn': function(){
		TurnOn((data) =>{
		var outputSpeech = data
		this.emit(':tell', outputSpeech);
		});
	},
  'TurnOff': function(){
		TurnOff((data) =>{
		var outputSpeech = data
		this.emit(':tell', outputSpeech);
		});
	},	
  'GetRoomTemp': function(){
		GetRoomTemp((data) =>{
		var outputSpeech = data
		this.emit(':tell', outputSpeech);
		});
	},	
  'SetThermostat': function(){
		tempSlot = this.event.request.intent.slots.temp.value;
		SetThermostat((data) =>{
		var outputSpeech = data
		this.emit(':tell', outputSpeech);
		});
	},	
  'GetThermostatSeting': function(){
		GetThermostatSeting((data) =>{
		var outputSpeech = data
		this.emit(':tell', outputSpeech);
		});
	},
  'SelfDiag': function(){
		//this.emit(':tell', "Diagnostic has started.");
		SelfDiag((data) =>{
		var outputSpeech = data
		this.emit(':tell', outputSpeech);
		});
	},	
  'GetUpTime': function(){
		GetUpTime((data) =>{
		var outputSpeech = data
		this.emit(':tell', outputSpeech);
		});
	},	
  'AMAZON.HelpIntent': function (){
      this.emit(':ask', "What can I help you with?", "How can I help?");
  },
  'AMAZON.CancelIntent': function (){
      this.emit(':tell', "Okay!");
  },
  'AMAZON.StopIntent': function (){
      this.emit(':tell', "Goodbye!");
  }};
//-----Function Section-----
function SpanishTest(callback) {
  //http://api.open-notify.org/astros.json
  var options = {
    host: 'lasverbenas.hopto.org',
    port: 81,
    path: '/SpanishTest',
    method: 'GET'
  };
  var req = http.request(options, res => {
      res.setEncoding('utf8');
      var returnData = "";

      res.on('data', chunk => {
          returnData = returnData + chunk;
      });

      res.on('end', () => {
        //var result = JSON.parse(returnData);
		var result = returnData;
        callback(result);

      });

  });
  req.end();
}
function TurnOn(callback) {
  //http://api.open-notify.org/astros.json
  var options = {
    host: 'lasverbenas.hopto.org',
    port: 81,
    path: '/on',
    method: 'GET'
  };
  var req = http.request(options, res => {
      res.setEncoding('utf8');
      var returnData = "";

      res.on('data', chunk => {
          returnData = returnData + chunk;
      });

      res.on('end', () => {
        //var result = JSON.parse(returnData);
		var result = returnData;
        callback(result);

      });

  });
  req.end();
}
function TurnOff(callback) {
  //http://api.open-notify.org/astros.json
  var options = {
    host: 'lasverbenas.hopto.org',
    port: 81,
    path: '/off',
    method: 'GET'
  };

  var req = http.request(options, res => {
      res.setEncoding('utf8');
      var returnData = "";

      res.on('data', chunk => {
          returnData = returnData + chunk;
      });

      res.on('end', () => {
        //var result = JSON.parse(returnData);
		var result = returnData;
        callback(result);

      });

  });
  req.end();
}
function GetRoomTemp(callback) {
  //http://api.open-notify.org/astros.json
  var options = {
    host: 'lasverbenas.hopto.org',
    port: 81,
    path: '/RoomTemp',
    method: 'GET'
  };

  var req = http.request(options, res => {
      res.setEncoding('utf8');
      var returnData = "";

      res.on('data', chunk => {
          returnData = returnData + chunk;
      });

      res.on('end', () => {
        //var result = JSON.parse(returnData);
		var result = returnData;
        callback(result);

      });

  });
  req.end();
}
function SetThermostat(callback) {
  //http://api.open-notify.org/astros.json
  var options = {
    host: 'lasverbenas.hopto.org',
    port: 81,
    path: '/SetTempSet?SetTempTo=' + tempSlot,
    method: 'GET'
  };

  var req = http.request(options, res => {
      res.setEncoding('utf8');
      var returnData = "";

      res.on('data', chunk => {
          returnData = returnData + chunk;
      });

      res.on('end', () => {
        //var result = JSON.parse(returnData);
		var result = returnData;
        callback(result);

      });

  });
  req.end();
}
function GetThermostatSeting(callback) {
  //http://api.open-notify.org/astros.json
  var options = {
    host: 'lasverbenas.hopto.org',
    port: 81,
    path: '/GetThermostatSeting',
    method: 'GET'
  };

  var req = http.request(options, res => {
      res.setEncoding('utf8');
      var returnData = "";

      res.on('data', chunk => {
          returnData = returnData + chunk;
      });

      res.on('end', () => {
        //var result = JSON.parse(returnData);
		var result = returnData;
        callback(result);

      });

  });
  req.end();
}
function SelfDiag(callback) {
  //http://api.open-notify.org/astros.json
  var options = {
    host: 'lasverbenas.hopto.org',
    port: 81,
    path: '/Diag',
    method: 'GET'
  };

  var req = http.request(options, res => {
      res.setEncoding('utf8');
      var returnData = "";

      res.on('data', chunk => {
          returnData = returnData + chunk;
      });

      res.on('end', () => {
        //var result = JSON.parse(returnData);
		var result = returnData;
        callback(result);

      });

  });
  req.end();
}
function GetUpTime(callback) {
  //http://api.open-notify.org/astros.json
  var options = {
    host: 'lasverbenas.hopto.org',
    port: 81,
    path: '/uptime',
    method: 'GET'
  };

  var req = http.request(options, res => {
      res.setEncoding('utf8');
      var returnData = "";

      res.on('data', chunk => {
          returnData = returnData + chunk;
      });

      res.on('end', () => {
        //var result = JSON.parse(returnData);
		var result = returnData;
        callback(result);

      });

  });
  req.end();
}






