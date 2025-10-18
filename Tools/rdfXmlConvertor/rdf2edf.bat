@echo off
title Rdf2Edf
java -javaagent:rdfXmlConvertor.jar="-pwd QQ1244453393" -jar rdfXmlConvertor.jar edf edf %1%
pause