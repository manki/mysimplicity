/**
 * Sends information about current time zone to watch. Can be used
 * by the watch to do rudimentary time zone conversions.
 */
function sendTimeZoneInfoToWatch() {
  var msg = {
    'timeZoneOffsetMinutes': new Date().getTimezoneOffset()
  };
  Pebble.sendAppMessage(msg,
      function() {
        console.log('Message sent successfully.');
      },
      function(e) {
        console.log('Message sending failed.');
      });
}

// Actions.
ACTION_GET_TIME_ZONE_OFFSET = 0;

function receiveAppMessage(req) {
  console.log('JS received message');
  var msg = req.payload;
  if (msg.action == ACTION_GET_TIME_ZONE_OFFSET) {
    sendTimeZoneInfoToWatch();
  }
}

Pebble.addEventListener('ready', function() {
  Pebble.addEventListener('appmessage', receiveAppMessage);
  // Send time zone info on ready so that watch face is initialised correctly.
  sendTimeZoneInfoToWatch();
});
