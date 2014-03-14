/**
 * Sends information about current time zone to watch. Can be used
 * by the watch to do rudimentary time zone conversions.
 */
function sendTimeZoneInfoToWatch() {
  var message = {
    'timeZoneOffsetMinutes': new Date().getTimezoneOffset()
  };
  Pebble.sendAppMessage(message,
      function() {
        console.log('Message sent successfully.');
      },
      function(e) {
        console.log('Message sending failed.');
      });
}

Pebble.addEventListener('ready', sendTimeZoneInfoToWatch);
