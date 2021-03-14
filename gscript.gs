function listCalendars() {
    var calendarIds = [];
    var calendars;
    var pageToken;

    do {
        calendars = Calendar.CalendarList.list({
            maxResults: 100,
            pageToken: pageToken
        });
        if (calendars.items && calendars.items.length > 0) {
            for (var i = 0; i < calendars.items.length; i++) {
                var calendar = calendars.items[i];
                calendarIds.push(calendar.id);
            }
        }
        pageToken = calendars.nextPageToken;
    } while (pageToken);

    return calendarIds;
}

function listNextEvents(calendarIds) {
    var now = new Date();
    var till = new Date(now.getTime() + (2 * 24 * 60 * 60 * 1000)); // two days ahead
    var allEvents = [];

    for (var calendarId of calendarIds) {
        var calendar = CalendarApp.getCalendarById(calendarId);
        var events = calendar.getEvents(now, till);

        for (var i = 0; i < events.length; i++) {
            var event = events[i];
            if (event.isAllDayEvent()) {
                // All-day event.
                var start = new Date(event.getAllDayStartDate())

                allEvents.push({
                    title: event.getTitle(),
                    start: start,
                    allDay: true
                });
            } else {
                var start = new Date(event.getStartTime());
                var end = new Date(event.getEndTime());

                allEvents.push({
                    title: event.getTitle(),
                    start: start,
                    end: end,
                    allDay: false
                });
            }
        }
    }

    allEvents.sort((a, b) => a.start - b.start);

    for (var i = 0; i < allEvents.length; i++) {
        if (allEvents[i].allDay) {
            allEvents[i].start = Utilities.formatDate(allEvents[i].start, "Australia/Sydney", "dd/MM/yyyy");
        } else {
            allEvents[i].start = Utilities.formatDate(allEvents[i].start, "Australia/Sydney", "dd/MM/yyyy HH:mm a");
            allEvents[i].end = Utilities.formatDate(allEvents[i].end, "Australia/Sydney", "dd/MM/yyyy HH:mm a");
        }
    }

    return allEvents;
}

function doGet() {
    var calendarIds = listCalendars();
    var nextEvents = listNextEvents(calendarIds);

    return ContentService.createTextOutput(JSON.stringify(nextEvents))
        .setMimeType(ContentService.MimeType.JSON);
}


