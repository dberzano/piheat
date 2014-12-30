## @file timestamp.py
#  Contains a class definition for timestamp handling.

import calendar, datetime

## @class TimeStamp
#  Time stamp handling in Python for human beings.
#
#  Python handles dates like crazy. This class is constructed from a timestamp, holds a naive
#  datetime and has methods with crystal clear names. Small but sufficient for our purposes.
class TimeStamp:

  # datefmt = Enum({
  #   'NO_USEC': '%Y-%m-%d %H:%M:%S',
  #   'DATE_ONLY': '%Y-%m-%d',
  #   'TIME_ONLY': '%H:%M:%S'
  # })

  ## Initializes this `TimeStamp` instance from a UTC time stamp.
  #
  #  @param ts_utc A number with a timestamp treated as if it were in the UTC timezone, or `None` to
  #                initialize the object with current timestamp
  def __init__(self, ts_utc=None):
    if ts_utc is None:
      self._dt_utc = datetime.datetime.utcnow()
    else:
      self._dt_utc = datetime.datetime.utcfromtimestamp(ts_utc)

  ## Initializes this `TimeStamp` from an ISO-formatted UTC date.
  #
  #  @param isodate A string with the ISO-formatted date in the UTC timezone
  @classmethod
  def from_iso_str(cls, isodate):
    iso_to_dt = datetime.datetime.strptime(isodate, '%Y-%m-%dT%H:%M:%S.%fZ')
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

  ## String representation of a date.
  #
  #  @return A string representation of the current instance
  def __str__(self):
    #return self._dt_utc.strftime('%Y-%m-%e %H:%M:%S')
    return str(self._dt_utc)

  ## Subtracts two `TimeStamp` objects.
  #
  #  Subtract order: `self - other`. You can get the seconds with `<timedelta>.total_seconds()`.
  #
  #  @param other A `TimeStamp` object to subtract to `self`
  #
  #  @return A `timedelta` object.
  def __sub__(self, other):
    return datetime.timedelta(seconds=self.get_timestamp_usec_utc()-other.get_timestamp_usec_utc())

  ## A simple unit test to test if this class works.
  @staticmethod
  def assert_unit_test():
    tsraw = 1406754375.6
    tsstr = '2014-07-30 21:06:15.600000'
    tsobj = TimeStamp(1406754375.6)
    assert tsobj.get_timestamp_usec_utc() == tsraw, 'UTC timestamps does not correspond'
    assert str(tsobj) == tsstr, 'UTC string representations do not correspond'
