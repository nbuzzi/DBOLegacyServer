@echo off
title Edf2Rdf
java -javaagent:rdfXmlConvertor.jar="-pwd QQ1244453393" -jar rdfXmlConvertor.jar edf rdf %1%
pause