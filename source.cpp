#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <optional>
#include <set>
#include <functional>

#include "pugixml/pugixml.hpp"

struct dictionnary {
	struct word_info {
		std::string hatsuon;
		std::string translation;
		int ent_seq = 0;
		bool is_suru_verb;
		bool is_na_adj;
		bool is_t1; // jidoushi
		bool is_t2; // tadoushi
	};
	std::unordered_map<std::string, word_info> data;
};

std::string to_complete_dict(const std::string& word, const dictionnary& dict) {
	if (dict.data.find(word) == dict.data.end())
		return word;
	
	auto complete = dict.data.at(word);
	return word + (complete.is_suru_verb ? "[する]" : "")
		+ (complete.is_na_adj ? "[な]" : "")
		+ (complete.is_t1 ? "[T1]" : "")
		+ (complete.is_t2 ? "[T2]" : "")
		+ "（" + complete.hatsuon + "）: " + complete.translation;
}

struct kanji {
	uint32_t number = 0;
	uint32_t level = 0;
	std::string kanji;
	std::string category;
	struct {
		std::vector<std::string> reads;
		struct {
			std::vector<std::string> important, added;
		} examples;
	} kunyomi, onyomi;

	std::string to_string(const dictionnary& dict) const {
		if (kanji.size() == 0)
			return "";
		std::string str = "漢字::" + category + '\t' + "<h1 style=\"font-weight: normal\">" + kanji + "</h1>" + '\t';
		if (!kunyomi.reads.empty()) {
			for(const auto& read : kunyomi.reads)
				str += read + ", ";
			str += "<br>";
		}
		if (!onyomi.reads.empty()) {
			for(const auto& read : onyomi.reads)
				str += read + ", ";
			str += "<br>";
		}
		if (!kunyomi.examples.important.empty()) {
			for(const auto& ex : kunyomi.examples.important)
				str += "<br>" + to_complete_dict(ex, dict);
		}
		if (!onyomi.examples.important.empty()) {
			for(const auto& ex : onyomi.examples.important)
				str += "<br>" + to_complete_dict(ex, dict);
		}
		if (!kunyomi.examples.added.empty() || !onyomi.examples.added.empty()) str += "<br>";
		if (!kunyomi.examples.added.empty()) {
			for(const auto& ex : kunyomi.examples.added)
				str += "<br>" + to_complete_dict(ex, dict);
		}
		if (!onyomi.examples.added.empty()) {
			for(const auto& ex : onyomi.examples.added)
				str += "<br>" + to_complete_dict(ex, dict);
		}
		return str;
	}

	// Not yet existing functionnality
	int get_jlpt_level() const {
		return 0;
	}
};

std::optional<std::vector<std::string>> absent_words_from_dict(const std::vector<std::string>& words, const dictionnary& dict) {
	std::vector<std::string> rtv;
	for(const auto& word : words) {
		if (!(word.contains("（") && word.contains("）")) && !dict.data.contains(word)) rtv.push_back(word);
	}
	if (rtv.size() > 0) return rtv;
	return std::nullopt;
}

std::optional<std::vector<std::string>> absent_kanji_from_words(const std::vector<std::string>& words, const std::string& knj) {
	std::vector<std::string> rtv;
	for(const auto& word : words) {
		if (!(word.contains("（") && word.contains("）")) && !word.contains(knj)) rtv.push_back(word);
	}
	if (rtv.size() > 0) return rtv;
	return std::nullopt;
}

std::optional<std::string> check_kanji(const kanji& k, const dictionnary& dict) {
	std::vector<std::string> missing_words;
	std::vector<std::string> missing_kanji_words;

	const std::vector<std::string>* word_vtors[4] = {
		&k.kunyomi.examples.important,
		&k.kunyomi.examples.added,
		&k.onyomi.examples.important,
		&k.onyomi.examples.added
	};

	for(const auto& wordlist : word_vtors) {
		auto result_words = absent_words_from_dict(*wordlist, dict);
		auto result_use = absent_kanji_from_words(*wordlist, k.kanji);
		if (result_use.has_value()) {
			missing_kanji_words.insert(missing_kanji_words.end(), result_use->begin(), result_use->end());
		}
		if (result_words.has_value()) {
			missing_words.insert(missing_words.end(), result_words->begin(), result_words->end());
		}
	}
	if (missing_kanji_words.empty() & missing_words.empty()) return std::nullopt;

	// Create error message
	std::string err = "Kanji " + k.kanji + " (" + std::to_string(k.number) + ") :\n";
	for(const auto& a : missing_kanji_words) {
		err += "\tkanji missing in word " + a + '\n';
	}
	for(const auto& a : missing_words) {
		err += "\tword " + a + " missing from dictionnary\n";
	}

	return err;
}

std::vector<std::string> split_2(std::string s, const std::string& delimiter) {
    std::vector<std::string> tokens;
	if (s.empty()) return tokens;
	if (s.starts_with('"') && s.ends_with('"')) {
		s = s.substr(1, s.size()-2);
	}
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

std::optional<kanji> read_kanji2(std::ifstream& in) {
	// Comma positioning
	std::vector<size_t> comma_pos;

	kanji k;
	std::string line;
	std::getline(in, line);
	if (line.empty()) {
		return std::nullopt;
	}

	{
		bool is_in_quotes = false;
		for(size_t i = 0; i < line.size(); ++i) {
			if (line.at(i) == '"') is_in_quotes = !is_in_quotes;
			if (line.at(i) == ',' && !is_in_quotes) comma_pos.push_back(i);
		}
	}

	if (comma_pos.size() < 9) return std::nullopt;

	auto get_substr = [&comma_pos, &line](const int pos) -> std::string {
		return line.substr(comma_pos.at(pos-1)+1,(pos < comma_pos.size()) ? (comma_pos.at(pos) - comma_pos.at(pos-1) - 1) : std::string::npos);
	};

	k.number = std::stoi(line.substr(0, comma_pos.at(0)));
	k.kanji = get_substr(2);
	k.category = get_substr(3);
	k.kunyomi.reads = split_2(get_substr(4), ";");
	k.onyomi.reads = split_2(get_substr(5), ";");
	k.kunyomi.examples.important = split_2(get_substr(6), ";");
	k.onyomi.examples.important = split_2(get_substr(7), ";");
	k.kunyomi.examples.added = split_2(get_substr(8), ";");
	k.onyomi.examples.added = split_2(get_substr(9), ";");
	k.level = k.get_jlpt_level();
	return k;
}

std::vector<kanji> read_file(const std::string& filename) {
	std::vector<kanji> list;
	std::ifstream in(filename);
	if (!in.is_open()) {
		return list;
	}
	while(in) {
		auto kanji = read_kanji2(in);
		if (kanji.has_value())
			list.push_back(kanji.value());
	}
	return list;
}

bool write_anki_file(const std::string& filename, const std::vector<kanji>& list, const dictionnary& dict) {
	std::ofstream out(filename);
	if (!out.is_open()) return false;
	out << "#separator:tab\n#html:true\n#deck column:1\n#tags column:4\n";

	size_t word_count = 0;
	for(const auto& kanji : list) {
		auto check_result = check_kanji(kanji, dict);
		if (check_result.has_value()) {
			std::cout << check_result.value();
		}
		if (!kanji.kanji.empty())
			out << kanji.to_string(dict) << '\n';
		
		word_count += kanji.kunyomi.examples.important.size() +
			kanji.kunyomi.examples.added.size() +
			kanji.onyomi.examples.important.size() +
			kanji.onyomi.examples.added.size();
	}
	std::cout << "Kanji word count " << list.size() << '\n';
	std::cout << "Vocabulary word count " << word_count << '\n';

	return true;
}

std::vector<std::string> get_keb_list(const pugi::xml_node& node) {
	std::vector<std::string> kebvals;
	auto k_eles = node.children("k_ele");
	for(const auto& kele : k_eles) {
		auto kebs = kele.children("keb");
		for(const auto& keb : kebs) {
			kebvals.push_back(std::string(keb.child_value()));
		}
	}
	return kebvals;
}

std::optional<dictionnary> read_dictionnary(const std::string& filename) {
	pugi::xml_document doc;
	auto status = doc.load_file(filename.c_str());
	if (status.status != pugi::xml_parse_status::status_ok) {
		throw std::runtime_error("xml_parse_status::" + std::to_string(status));
	}

	// Getting jmdict object
	auto jmdict = doc.child("JMdict");
	std::cout << "jmdict name " << jmdict.name() << std::endl;

	// Parsing through the jmdict objects
	auto entries = jmdict.children();
	dictionnary dict;

	for(const auto& entry : entries) {
		std::string ent_seq_str = entry.child_value("ent_seq");
		int ent_seq = (ent_seq_str.empty() ? 0 : std::stoi(ent_seq_str));
		auto kebs = get_keb_list(entry);
		auto reb = entry.child("r_ele").child_value("reb");
		auto sense = entry.child("sense").children("gloss");
		auto word_type = entry.child("sense").children("pos");
		std::string senses;
		if (sense.empty()) {
			senses = "no translation";
		} else {
			auto last = sense.end();
			--last;
			for(const auto& s : sense) {
				senses += s.child_value();
				if (s != *last)
					senses += ", ";
			}
		}
		bool is_suru_verb = false;
		bool is_na_adj = false;
		bool is_t1 = false, is_t2 = false;
		if (!word_type.empty()) {
			for(const auto& type : word_type) {
				auto str = std::string(type.child_value());
				is_suru_verb |= str == "&vs;";
				is_na_adj |= str == "&adj-na;";
				is_t1 |= str == "&vi;";
				is_t2 |= str == "&vt;";
			}
		}
		dictionnary::word_info kinf;
		kinf.hatsuon = reb;
		kinf.translation = senses;
		kinf.ent_seq = ent_seq;
		kinf.is_suru_verb = is_suru_verb;
		kinf.is_na_adj = is_na_adj;
		kinf.is_t1 = is_t1;
		kinf.is_t2 = is_t2;
		for(const auto& keb : kebs) {
			auto alreadyreg = dict.data.find(keb);
			if (alreadyreg == dict.data.end()) {
				dict.data[keb] = kinf;
			} else {
				if (alreadyreg->second.ent_seq > kinf.ent_seq) {
					dict.data[keb] = kinf;
				}
			}
		}
	}
	std::cout << "Dictionnary has been read, " << dict.data.size() << " entries added" << std::endl;
	return dict;
}


std::set<std::string> get_vocab_from_kanji_list(const std::vector<kanji> list) {
	std::set<std::string> rtval;
	for(const auto& k : list) {
		std::copy(k.kunyomi.examples.important.begin(), k.kunyomi.examples.important.end(), std::inserter(rtval, rtval.end()));
		std::copy(k.kunyomi.examples.added.begin(), k.kunyomi.examples.added.end(), std::inserter(rtval, rtval.end()));
		std::copy(k.onyomi.examples.important.begin(), k.onyomi.examples.important.end(), std::inserter(rtval, rtval.end()));
		std::copy(k.onyomi.examples.added.begin(), k.onyomi.examples.added.end(), std::inserter(rtval, rtval.end()));
	}
	return rtval;
}

int mainpp(const std::vector<std::string> args) {
	// 1st arg : db file
	// 2nd arg : dictionnary file
	// 3rd arg : output file (optional)

	auto list = read_file(args.at(1));
	auto dictionnary = read_dictionnary(args.at(2));
	std::string outfile = "out.txt";
	if (args.size() > 3) {
		outfile = args.at(3);
	}
	return write_anki_file(outfile, list, dictionnary.value());
}

int main(int argc, char** argv) {
	std::vector<std::string> args;
	for(int i = 0; i < argc; ++i) {
		args.push_back(argv[i]);
	}
	return mainpp(args);
}