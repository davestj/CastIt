use scastd;
# MySQL dump 8.13
#
# Host: localhost    Database: scastd
#--------------------------------------------------------
# Server version	3.23.36

#
# Table structure for table 'scastd_memberinfo'
#

CREATE TABLE scastd_memberinfo (
  serverURL varchar(255) NOT NULL default '0',
  password varchar(155) NOT NULL default '',
  gather_flag varchar(155) NOT NULL default '',
  PRIMARY KEY  (serverURL)
) TYPE=MyISAM;

#
# Dumping data for table 'scastd_memberinfo'
#

INSERT INTO scastd_memberinfo VALUES ('http://shoutcast.mediacast1.com:8015','','1');
INSERT INTO scastd_memberinfo VALUES ('http://shoutcast.mediacast1.com:8000','','1');
INSERT INTO scastd_memberinfo VALUES ('http://shoutcast.mediacast1.com:8185','','1');

#
# Table structure for table 'scastd_runtime'
#

CREATE TABLE scastd_runtime (
  sleeptime int(11) default NULL,
  logfile varchar(255) default NULL
) TYPE=MyISAM;

#
# Dumping data for table 'scastd_runtime'
#

INSERT INTO scastd_runtime VALUES (30,'~/scastd/scastd.log');

#
# Table structure for table 'scastd_serverinfo'
#

CREATE TABLE scastd_serverinfo (
  serverURL varchar(255) default NULL,
  currentlisteners int(11) default NULL,
  peaklisteners int(11) default NULL,
  maxlisteners int(11) default NULL,
  averagetime int(11) default NULL,
  streamhits int(11) default NULL,
  time timestamp(14) NOT NULL
) TYPE=MyISAM;

#
# Dumping data for table 'scastd_serverinfo'
#


#
# Table structure for table 'scastd_songinfo'
#

CREATE TABLE scastd_songinfo (
  serverURL varchar(255) default NULL,
  songTitle varchar(255) default NULL,
  time timestamp(14) NOT NULL
) TYPE=MyISAM;

#
# Dumping data for table 'scastd_songinfo'
#


#
# Table structure for table 'scastd_xml'
#

CREATE TABLE scastd_xml (
  id int(10) NOT NULL auto_increment,
  currentlisteners varchar(155) NOT NULL default '',
  peaklisteners varchar(155) NOT NULL default '',
  maxlisteners varchar(155) NOT NULL default '',
  reportedlisteners varchar(155) NOT NULL default '',
  averagetime varchar(255) NOT NULL default '',
  servergenre varchar(155) NOT NULL default '',
  serverurl varchar(255) NOT NULL default '',
  servertitle varchar(255) NOT NULL default '',
  currentsong varchar(255) NOT NULL default '',
  webhits varchar(155) NOT NULL default '',
  streamhits varchar(255) NOT NULL default '',
  hostname varchar(255) NOT NULL default '',
  useragent varchar(255) NOT NULL default '',
  connecttime varchar(155) NOT NULL default '',
  songtitle varchar(255) NOT NULL default '',
  playedat varchar(55) NOT NULL default '',
  bwidthus varchar(55) NOT NULL default '',
  totbwidthus varchar(255) NOT NULL default '',
  date date NOT NULL default '0000-00-00',
  time time NOT NULL default '00:00:00',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

#
# Dumping data for table 'scastd_xml'
#


