# kanji-csv-to-anki-cards-builder
Building Anki flashcards from a csv file, using a dictionary file

# Usage

kanji-anki-card-builder database_file.csv dictionary_file.xml [output_file.txt]

database_file.csv is the CSV file containing all the kanjis that will be included in the final flashcard files.

dictionary_file.xml is the JMdict_e file that will be used as resource to get data.

output_file.txt is the optional file name for the output flashcard file.

# License

This work is available to anybody free of charge, under the terms of MIT License

This software is based on pugixml library (http://pugixml.org). pugixml is Copyright (C) 2006-2025 Arseny Kapoulkine.

This package uses the JMdict/EDICT (https://www.edrdg.org/wiki/index.php/JMdict-EDICT_Dictionary_Project) dictionary files. These files are the property of the Electronic Dictionary Research and Development Group (https://www.edrdg.org/), and are used in conformance with the Group's licence (https://www.edrdg.org/edrdg/licence.html).

Specifically, it uses the JMdict_e file as dictionary resource get the pronunciations and definitions of words.
