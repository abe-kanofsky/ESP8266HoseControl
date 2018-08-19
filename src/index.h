const char PAGE[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
	<head>
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Hose Control</title>
		<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/foundicons/3.0.0/foundation-icons.css">

		<script src="https://code.jquery.com/jquery-3.3.1.min.js""></script>
		<script src="https://cdnjs.cloudflare.com/ajax/libs/foundation/6.5.0-rc.2/js/foundation.min.js"></script>
		<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/foundation/6.5.0-rc.2/css/foundation.min.css">

		<style>
			.card {
				width: 450px;
			}
			.grid-x {
				padding-left: 35px;
				padding-right: 70px;
			}
			.tooltip {
			    max-width: 90%;
			    width: 90%;
			}

		</style>
		
		<script type="text/javascript">
			$(document).ready(function() {$(document).foundation();})

			var ws = new WebSocket("ws://" + window.location.hostname)
			var responseTimeout = null

			ws.onerror = function() {
				console.log("onerror triggered")
				setTimeout(location.reload, 2000)
			}
			ws.onclose = function() {
				location.reload()
			}

			ws.onmessage = function (evt) {
				if (responseTimeout) {
					clearTimeout(responseTimeout)
				}
				console.log(evt.data)
				if (evt.data == "pong") { return }
				var s = evt.data.split("/")
				console.log(s)
				if (s[0] == "countdown-time-left") {
					updateTimer(s[1])
				} else {
					document.getElementById(s[0])[s[1]] = s[2]
				}
			}

			console.log("date(): " + new Date().getTime())
			var countdownTimerInterval = null
			function updateTimer(secondsRemaining) {
				var outputElem = document.getElementById("countdown-display")
				clearInterval(countdownTimerInterval)
				console.log("seconds remaining: " + secondsRemaining)

				countdownTimerInterval = setInterval( function () {
					if (secondsRemaining <= 0) {
						outputElem.innerHTML = ""
						clearInterval(countdownTimerInterval)
					} else {
						var s = (secondsRemaining % 60).toString().padStart(2, '0')
						var m = ((secondsRemaining - s) / 60).toString().padStart(2, '0')
						outputElem.innerHTML = "Time remaining: " + m + ":" + s
						secondsRemaining --
					}
				}, 1000)
			}

			function updateValue(id, value) {
				console.log(" updating value ")
				var msg = id + "/" + value
				console.log("sending message: " + msg)
				ws.send(msg)
				awaitResponse()
			}

			function awaitResponse() {
				if (responseTimeout) {
					clearTimeout(responseTimeout)
				}
				responseTimeout = setTimeout(function() {
					alert("Lost connection to controller. Page will be reloaded.")
					location.reload()
				}, 3000)
			}

			function addCountdownTime() {
				ws.send()
			}

			window.onfocus = function () {
				ws.send("ping")
				awaitResponse()
			}
		</script> 
	</head>
	<body>
		<h1>Hose Control</h1>
		<div class="card">
			<div class="card-divider">
				<h5 style="display: block; width: 100%;">Main Control
    				<i class="fi-info large"  style="float: right;" data-tooltip 
    				title="Turn the hose on or off."></i>
		    	</h5>
			</div>
			<div class="card-section">
				<div class="grid-x grid-padding-x odd">
					<div class="cell auto">ON/OFF</div>
		 		 	<div class="cell shrink">
			 		 	<div class="switch large">
							<input class="switch-input" type="checkbox" id="state" onchange="updateValue(this.id, this.checked)">
							<label class="switch-paddle" for="state">
								<span class="show-for-sr"></span>
							</label>
						</div>
					</div>
			  	</div>
			</div>
		</div>

	    <div class="card">
			<div class="card-divider">
		    	<h5 style="display: block; width: 100%;">Countdown Timer
    				<i class="fi-info large"  style="float: right;" data-tooltip
    				title="Pressing this button will turn on the hose and then automatically turn it off after five minutes. If the countdown timer is already active, pressing the button will add 5 minutes to the timer. To turn the hose off while the timer is active, simply use the main ON/OFF switch above."></i>
		    	</h5>
		    </div>
	   		<div class="card-section">
				<div class="grid-x grid-padding-x grid-padding-y even">
					<div class="cell shrink">
				    	<button class="button" id="add-time" data-time="5" onclick='updateValue(this.id, this.getAttribute("data-time"))'>+5 mins</button>
					</div>
					<div class="cell auto settings-label" id="countdown-display"></div>
				</div>
			</div>
	    </div>

		<div class="card">
			<div class="card-divider">
				<h5 style="display: block; width: 100%;">Daily Timer
    				<i class="fi-info large"  style="float: right;" data-tooltip
    				title="When this feature is enabled, the hose will automatically turn on and off each day at the specified times."></i>
		    	</h5>
			</div>
			<div class="card-section">
				<div class="grid-y">
					<div class="grid-x grid-padding-x even">
						<div class="cell auto settings-label">ON/OFF</div>
						<div class="cell shrink">
				 		 	<div class="switch small">
				    			<input class="switch-input" type="checkbox" id="timer-enable" onchange="updateValue(this.id, this.checked)">
								<label class="switch-paddle" for="timer-enable">
									<span class="show-for-sr"></span>
								</label>
							</div>
						</div>
				  	</div>

				  	<div class="grid-x grid-padding-x odd">
						<div class="cell auto settings-label">Start Time</div>
				  		<div class="cell shrink">
							<input type="time" id="start-time" min="0:00" max="23:59" onchange="updateValue(this.id, this.value)" />
				  		</div>
				  	</div>
				  	<div class="grid-x grid-padding-x even">
						<div class="cell auto settings-label">End Time</div>
				  		<div class="cell shrink">
							<input type="time" id="end-time" min="0:00" max="23:59" onchange="updateValue(this.id, this.value)" />
				  		</div>
				  	</div>
			  	</div>
			</div>
		</div>

	    <div class="card">
			<div class="card-divider">
		    	<h5 style="display: block; width: 100%;">Weather Checking
    				<i class="fi-info large"  style="float: right;" data-tooltip
    				title="When this feature is enabled, the forecast will be obtained each day, and if it predicts more than the specified amount of rain over the specified number of days, then the daily timer will be temporarily disabled."></i>
		    	</h5>
		    </div>
	   		<div class="card-section">
				<div class="card-y">
					<div class="grid-x grid-padding-x odd">
						<div class="cell auto settings-label">ON/OFF</div>
			 		 	<div class="cell shrink">
				 		 	<div class="switch small">
								<input class="switch-input" type="checkbox" id="weather-enabled" onchange="updateValue(this.id, this.checked)">
								<label class="switch-paddle" for="weather-enabled">
									<span class="show-for-sr"></span>
								</label>
							</div>
						</div>
			  		</div>
			  		<div class="grid-x grid-padding-x">
			  			<div class="cell auto settings-label">Days To Check (max 5)</div>
			  			<div class="cell shrink">
			  				<input id="weather-days" type="number" min="1" max="5" onchange="updateValue(this.id, this.value)">
			  			</div>
					</div>
					<div class="grid-x grid-padding-x">
			  			<div class="cell auto settings-label">Max Allowable Rain (mm)</div>
			  			<div class="cell shrink">
			  				<input id="max-rain" type="number" min="1" max="255" onchange="updateValue(this.id, this.value)">
			  			</div>
					</div>
				</div>
			</div>
	    </div>
	</body>
</html>

)=====";
