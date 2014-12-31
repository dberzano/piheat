## @file timestamp.py
#  Contains a class definition for timestamp handling.

import calendar, datetime

## @class TimeStamp
#  Time stamp handling in Python for human beings.
#
#  Python handles dates like crazy. This class is constructed from a timestamp, holds a naive
#  datetime and has methods with crystal clear names. Small but sufficient for our purposes.
class TimeStamp:

  ## Predefined date-time output formats
  format = {
    'NO_USEC': '%Y-%m-%d %H:%M:%S',
    'DATE_ONLY': '%Y-%m-%d',
    'TIME_ONLY': '%H:%M:%S',
    'ISO_FULL': '%Y-%m-%dT%H:%M:%S.%fZ'
  }


  ## Initializes this `TimeStamp` instance from a UTC time stamp.
  #
  #  @param ts_utc A number with a timestamp treated as if it were in the UTC timezone, or `None` to
  #                initialize the object with current timestamp
  #
  #  @exception TypeError Raised if parameter is not a number
  def __init__(self, ts_utc=None):
    if ts_utc is None:
      self._dt_utc = datetime.datetime.utcnow()
    else:
      self._dt_utc = datetime.datetime.utcfromtimestamp(ts_utc)


  ## Initializes this `TimeStamp` from an ISO-formatted UTC date.
  #
  #  This is an ISO-formatted UTC string:
  #
  #  ~~~{.txt}
  #  2014-07-30T21:06:15.600000Z
  #  ~~~
  #
  #  Note the `Z` that indicates the UTC timezone, and the `T` separating date from time.
  #
  #  In JavaScript, an ISO-formatted UTC string can be obtained from a `Date` object like this:
  #
  #  ~~~{.js}
  #  var d = new Date();
  #  console.log( d.toISOString() );
  #  ~~~
  #
  #  @param isodate A string with the ISO-formatted date in the UTC timezone
  #
  #  @exception ValueError Raised if string is malformed (not ISO-formatted UTC)
  #  @exception TypeError  Raised if parameter is not a string
  @classmethod
  def from_iso_str(cls, isodate):
    iso_to_dt = datetime.datetime.strptime(isodate, TimeStamp.format['ISO_FULL'])
    return cls( calendar.timegm( iso_to_dt.utctimetuple() ) + iso_to_dt.microsecond * 0.000001 )


  ## Gets current timestamp, including microseconds, in UTC.
  #
  #  @return A `Float` with the current timestamp in UTC
  def get_timestamp_usec_utc(self):
    return calendar.timegm( self._dt_utc.utctimetuple() ) + self._dt_utc.microsecond * 0.000001


  ## Gets a naive `datetime` object with this date in UTC.
  #
  #  Note that *naive* means that no timezone information will be available in the returned
  # `datetime` object: you'll have to treat it manually as UTC.
  #
  #  @return A naive `datetime` object
  def get_datetime_naive_utc(self):
    return self._dt_utc


  ## Gets a string with a formatted date. See
  #  [here](https://docs.python.org/2/library/time.html#time.strftime) for the format string
  #  description.
  #
  #  @param format A format string
  #
  #  @return A customly formatted string representation of the current instance
  def get_formatted_str(self, format):
    return self._dt_utc.strftime(format)


  ## ISO-formatted UTC string representation of the date.
  #
  #  @return A string representation of the current instance
  #
  #  @see from_iso_str
  def __str__(self):
    return self.get_formatted_str(TimeStamp.format['ISO_FULL'])


  ## Subtracts two `TimeStamp` objects.
  #
  #  Subtract order: `self - other`. You can get the seconds with `<timedelta>.total_seconds()`.
  #
  #  @param other A `TimeStamp` object to subtract to `self`
  #
  #  @return A `timedelta` object.
  def __sub__(self, other):
    return datetime.timedelta(seconds=self.get_timestamp_usec_utc()-other.get_timestamp_usec_utc())


  ## Check if functionalities provided by this class work.
  @staticmethod
  def assert_unit_test():
    tsraw = 1406754375.6
    tsstr = '2014-07-30T21:06:15.600000Z'
    tsnou = '2014-07-30 21:06:15'

    # constructing object from a UTC timestamp number
    for v in [ tsraw, tsstr ]:

      if isinstance(v, str):
        label = 'From ISO string'
        tsobj = TimeStamp.from_iso_str(v)
      else:
        label = 'From timestamp'
        tsobj = TimeStamp(v)

      assert tsobj.get_timestamp_usec_utc() == tsraw, \
        '%s: UTC timestamps mismatch: %ld != %ld' % (label, tsobj.get_timestamp_usec_utc(), tsraw)
      print '%s: timestamps test OK' % label

      assert str(tsobj) == tsstr, \
        '%s: UTC strings mismatch: %s != %s' % (label, str(tsobj), tsstr)
      print '%s: strings test OK' % label

      s = tsobj.get_formatted_str(TimeStamp.format['NO_USEC'])
      assert s == tsnou, \
        '%s: UTC strings without usec mismatch: %s != %s' % (s, tsnou)
      print '%s: strings without usec test OK' % label

    print 'All tests OK'


if __name__ == '__main__':
  TimeStamp.assert_unit_test()
