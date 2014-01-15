-- phpMyAdmin SQL Dump
-- version 3.3.7deb7
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Jan 08, 2014 at 12:42 PM
-- Server version: 5.1.72
-- PHP Version: 5.3.3-7+squeeze17

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `openflow_users`
--

-- --------------------------------------------------------

--
-- Table structure for table `capabilities`
--

CREATE TABLE IF NOT EXISTS `capabilities` (
  `Name` varchar(15) NOT NULL,
  `Description` varchar(100) NOT NULL,
  `Admin` tinyint(1) NOT NULL,
  `Demo` tinyint(1) NOT NULL,
  `Readonly` tinyint(1) NOT NULL,
  PRIMARY KEY (`Name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `capabilities`
--

INSERT INTO `capabilities` (`Name`, `Description`, `Admin`, `Demo`, `Readonly`) VALUES
('RESERVED', 'Capability to access reserved areas, like the Control Panel', 1, 0, 0),
('GET', 'Capability to access resources which require method GET', 1, 1, 1),
('ROUTE', 'Capability to access the routing resources', 1, 1, 0),
('DELETE', 'Capability to access resources which require method DELETE', 1, 0, 0),
('POST', 'Capability to access resources which require method POST', 1, 1, 0),
('SYNC', 'Capability to access the sync resources', 1, 1, 0),
('STAT', 'Capability to access the stat resources', 1, 1, 0);

-- --------------------------------------------------------

--
-- Table structure for table `editable_roles`
--

CREATE TABLE IF NOT EXISTS `editable_roles` (
  `Role` varchar(15) NOT NULL,
  `Cap` varchar(15) NOT NULL,
  PRIMARY KEY (`Role`,`Cap`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `editable_roles`
--

INSERT INTO `editable_roles` (`Role`, `Cap`) VALUES
('route', 'GET'),
('route', 'ROUTE'),
('StatManager', 'DELETE'),
('StatManager', 'GET'),
('StatManager', 'POST'),
('StatManager', 'ROUTE'),
('StatManager', 'STAT'),
('StatManager', 'SYNC');

-- --------------------------------------------------------

--
-- Table structure for table `resources`
--

CREATE TABLE IF NOT EXISTS `resources` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `Path` varchar(100) NOT NULL,
  `Cap` varchar(15) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `id` (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=41 ;

--
-- Dumping data for table `resources`
--

INSERT INTO `resources` (`id`, `Path`, `Cap`) VALUES
(1, '/netic.v1/OFNIC/synchronize', 'GET'),
(2, '/netic.v1/OFNIC', 'GET'),
(3, '/netic.v1/OFNIC/synchronize', 'SYNC'),
(4, '/netic.v1/OFNIC/synchronize/network', 'SYNC'),
(5, '/netic.v1/OFNIC/synchronize/network/all', 'SYNC'),
(6, '/netic.v1/OFNIC/statistics', 'STAT'),
(7, '/netic.v1/OFNIC/statistics/task', 'STAT'),
(8, '/netic.v1/OFNIC/statistics/task/create', 'STAT'),
(9, '/netic.v1/OFNIC/statistics/task/create', 'POST'),
(10, '/netic.v1/OFNIC/virtualpath', 'ROUTE'),
(11, '/netic.v1/OFNIC/virtualpath/create', 'ROUTE'),
(12, '/netic.v1/OFNIC/virtualpath/create', 'POST'),
(13, '/netic.v1/OFNIC/virtualpath', 'GET'),
(14, '/netic.v1/OFNIC/synchronize/network', 'GET'),
(15, '/netic.v1/OFNIC/synchronize/network/all', 'GET'),
(16, '/netic.v1/OFNIC/statistics', 'GET'),
(17, '/netic.v1/OFNIC/statistics/task', 'GET'),
(18, 'DELETE_STAT', 'STAT'),
(19, 'DELETE_STAT', 'DELETE'),
(20, 'GET_STAT', 'STAT'),
(21, 'GET_STAT', 'GET'),
(22, 'GET_NODE', 'STAT'),
(23, 'GET_NODE', 'GET'),
(24, 'GET_PATH', 'ROUTE'),
(25, 'GET_PATH', 'GET'),
(26, 'DELETE_PATH', 'ROUTE'),
(27, 'DELETE_PATH', 'DELETE'),
(28, 'CONTROL_PANEL', 'RESERVED'),
(29, 'SYNC_NODE', 'GET'),
(30, 'EXTENSION', 'GET'),
(31, '/netic.v1/extension', 'GET'),
(32, '/netic.v2/prova', 'GET'),
(33, '/netic.v2/limits', 'GET'),
(34, '/netic.v2/register', 'POST'),
(35, '/netic.v2/login', 'POST'),
(36, '/netic.v2/doc', 'GET'),
(37, '/netic.v1/limits', 'GET'),
(38, '/netic.v1/register', 'POST'),
(39, '/netic.v1/login', 'POST'),
(40, '/netic.v1/doc', 'GET');

-- --------------------------------------------------------

--
-- Table structure for table `roles`
--

CREATE TABLE IF NOT EXISTS `roles` (
  `Name` varchar(15) NOT NULL,
  `Editable` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`Name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `roles`
--

INSERT INTO `roles` (`Name`, `Editable`) VALUES
('Admin', 0),
('Demo', 0),
('No Access', 0),
('Readonly', 0),
('StatManager', 1),
('Superuser', 0),
('route', 1);

-- --------------------------------------------------------

--
-- Table structure for table `users`
--

CREATE TABLE IF NOT EXISTS `users` (
  `username` varchar(10) NOT NULL,
  `password` varchar(32) NOT NULL,
  `language` set('en','it') NOT NULL DEFAULT 'en',
  PRIMARY KEY (`username`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `users`
--

INSERT INTO `users` (`username`, `password`, `language`) VALUES
('routeDemo', '54984ae008e5934da82bc05480637f1a', 'en'),
('admin', 'd373b04866e52cc6d7b043383bf17ae4', 'en'),
('demo', '93bdad91cbe943875f2d433d3e99baf2', 'en');

-- --------------------------------------------------------

--
-- Table structure for table `user_roles`
--

CREATE TABLE IF NOT EXISTS `user_roles` (
  `User` varchar(10) NOT NULL,
  `Role` varchar(15) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `user_roles`
--

INSERT INTO `user_roles` (`User`, `Role`) VALUES
('admin', 'Superuser'),
('demo', 'Readonly'),
('routeDemo', 'Readonly'),
('routeDemo', 'route'),
('admin', 'Admin');
