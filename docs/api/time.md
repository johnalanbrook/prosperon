# time
Functions for manipulating time.
#### now()

Get the time now.

#### computer_dst()

Return true if the computer is in daylight savings.

#### computer_zone()

Get the time zone of the running computer.

#### hour2minute()



#### day2hour()



#### minute2second()



#### week2day()



#### strparse
**object**



#### second
**number**

Earth-seconds in a second.

#### minute
**number**

Seconds in a minute.

#### hour
**number**

Seconds in an hour.

#### day
**number**

Seconds in a day.

#### week
**number**

Seconds in a week.

#### weekdays
**array**

Names of the days of the week.

#### monthstr
**array**

Full names of the months of the year.

#### epoch
**number**

Times are expressed in terms of day 0 at hms 0 of this year.

#### isleap(year)

Return true if the given year is a leapyear.

#### yearsize(y)

Given a year, return the number of days in that year.

#### timecode(t, fps = 24)



#### monthdays
**array**

Number of days in each month.

#### zones
**object**



#### record(num, zone = this.computer_zone()

Given a time, return an object with time fields.

#### number(rec)

Return the number representation of a given time.

#### fmt
**string**

Default format for time.

#### text(num, fmt = this.fmt, zone)

Return a text formatted time.


