#include "apps/main_menu.hpp"

#include <stddef.h>
#include <string.h>

#include <sdk/terminal.hpp>

#include "ps2.hpp"
#include "vga.hpp"

#include "apps/components/menu.hpp"
#include "apps/components/pager.hpp"

#include "apps/snake.hpp"
#include "apps/mieliepit.hpp"
#include "apps/character_map.hpp"
#include "apps/editor.hpp"
#include "apps/uptime.hpp"
#include "apps/calculator.hpp"
#include "apps/queued_demo.hpp"
#include "apps/callback_demo.hpp"
#include "apps/ignore_demo.hpp"

using namespace term;

namespace main_menu {

namespace {

void pager_test() {
	const char *text =
		"This is a long text. And I should have at least a few multi-line paragraphs in here. As well as empty lines, obviously. The purpose of this text is to test the \"pager\" component to see how well it functions. (Or well, to see *that* it functions)\n"
		"\n"
		"Once a kernel was made by me,\n"
		"I thought it would not a success story be,\n"
		"Yet here we sit now,\n"
		"Enjoying its use,\n"
		"And I can think is \"wow\"!\n"
		"\n"
		"...\n"
		"...as in \"wow\" that's an awful poem. What am I, grade three? But a little whimsy never hurt nobody, so I think I'm alright. And it's not a uni project anymore, so no need to be serious and professional all the time \x01\n"
		"Anyways, what do you think of the poem?\n"
		"I know a few limericks and tongue-twisters I could write down, but idk what their copyright statuses are...\n"
		"Though I guess something like \"She sells sea shells by the sea shore.\" would almost certainly be public domain; I mean, I definitely heard it from multiple oral sources before I ever heard it written down.\n"
		"And then I guess some of the little tongue twisters I remember as a kid should be fine too. Try saying \"rooi kat, blou kat, geel kat\" three times fast \x01 Though of course, if you can't speak (or read) Afrikaans, or if you struggle with your g's then you'd probably not get the desired effect. Very funny though. And no I'm not gonna tell you why it's funny :P\n"
		"\n"
		"\n"
		"The quick brown fox jumps over the lazy dog.\n"
		"The quick brown fox jumps over the lazy dog.\n"
		"Lorum ipsum dolor set [...]\n"
		"Never gonna give you up, never gonna let you down, never gonna run around and desert you.\n"
		"Never gonna remember the rest of these lyrics either.\n"
		"\n"
		"\"Jy moet die Here jou God liefh\x88 met jou hele hart en met jou hele siel en met jou hele verstand.\" Dit is die belangrikste gebod. En die tweede wat hieraan gelyk staan, is, \"Jy moet jou naaste liefh\x88 soos jouself.\"\n"
		"\n"
		"= Taalverwarring by Babel =\n"
		"1 Die hele aarde het een taal gehad en dieselfde woorde.\n"
		"2 Met hulle trek na die ooste kom hulle toe af op 'n vallei in die land Sinar en gaan woon daar.\n"
		"3 Hulle het vir mekaar ges\x88: ``Kom ons maak kleinstene en bak dit hard.'' Die kleinstene word toe hulle boustene, en asfalt word hulle messelklei.\n"
		"4 Hulle s\x88 toe: ``Kom ons bou vir ons 'n stad en 'n toring met sy bopunt in die hemel, sodat ons vir onsself naam kan maak; anders versprei ons oor die hele aarde.''\n"
		"5 Maar die HERE het afgedaal om na die stad en die toring te kyk wat die mensekinders bou.\n"
		"6 Die HERE s\x88 toe: ``Kyk, dit is een volk, en hulle almal het een taal, en dit is maar die begin van wat hulle gaan doen. Nou sal niks van wat hulle beplan om te doen, vir hulle onmoontlik wees nie.\n"
		"7 Kom, laat Ons afgaan en hulle taal daar verwar, dat die een nie die ander se taal verstaan nie.''\n"
		"8 Die HERE het hulle daarvandaan oor die hele aarde versprei, en hulle het opgehou om die stad te bou.\n"
		"9 Daarom word dit Babel genoem, want daar het die HERE die taal van die hele aarde verwar. Daarvandaan het die HERE hulle oor die hele aarde versprei.\n"
		"(Gen. 11:1-9, 2020-vertaling)\n"
		"\n"
		"...en nou het ons meer as een skermvol se teks! Ongelukkig is dit minder as twee, so ek kan my page up en page down nie behoorlik toets nie... Die Bybelgenootskap wil ook h\x88 dat aanhaalings uit hulle bybelvertaling ook nie meer as 25% van die teks waarin dit voorkom opmaak nie, so ek moet so ongeveer agthonderd woorde skryf wat n\xa1""e bybelaanhalings is nie, aangesien ek rofweg tweehonderd sewentig woorde uit die bybel uit aangehaal het. Ek moet s\x88, die feit dat ek nou inelkgeval soveel woorde s\x82lf moet skryf laat my wonder of die Bybelaanhalings regtig die moeite werd is... Ek bedoel, net die liefdesgebod is seker fair use soos die Amerikaners dit noem, maar nege verse uit Genesis? Nou wil ek eerder seker maak ek bly by hulle voorgeskrewe re\x89ls.\n"
		"Maar ek weet nie, die Babelteks maak vir 'n goeie \"filler text\", en nou het ek 'n verskoning om Afrikaans in 'n Engelse projek te skryf. Jy sien, ek skryf nie net Afrikaans nie, ek's ook besig grappig te wees, om 'n \"easter egg\" in te sit deur om 'n taal te gebruik wat meeste van die gebruikers van die program (of lesers van sy kode) nie sal verstaan nie, reg na ek 'n Bybelteks aanhaal wat gaan oor taalverwarring en hoe mense mekaar ophou verstaan het ;p\n"
		"Dit is absoluut glad nie dat ek nou al die moeite gedoen het om die teks oor te tik en nou te hardkoppig is om daardie werk nou te skrap nie, tot die punt dat ek nou maar moet dink aan 'n verdere 800 woorde om te skryf, eerder as om net gou-gou vir 'n KI te vra om vir my filler te genereer, en dan ook my lisensieeringsinligting in my README aan te pas nie, nee nooit!\n"
		"\n"
		"Maar nou dat ek KI noem, wil ek net gou s\x88, ek het dit reggekry om 'n Qwen3 LLM met 1.7 miljard \"parameters\" op my ou laptoppie te hardloop, en al loop hy net teen so drie tokens per sekonde as ek niks anders doen nie, vind ek dit steeds vreeslik cool. Wel nog nie vreeslik baie nut uit hom gekry nie, ChatGPT is beide vinniger en slimmer, maar ek kan hom wel persoonlike inligting gee want ek weet hy stuur dit nie vir OpenAI om deur te lees nie. Ek moes entlik hom net vra vir 'n sample teks, dan wag ek net 'n minuut of twee, en dan het ek dit.\n"
		"Maar nou ja, ek het al vyf of ses of tien minute hier al gesit en goed uittik wat in my kop opkom, so nou is daar 'n bietjie van 'n sunk cost fallacy, so ek gaan maar aanhou wegtik totdat ek gelukkig is dat ek genoeg van 'n teks om die Bybelaanhalings geskep het dat dit binne aanvaarbare gebruik bly.\n"
		"\n"
		"Jy weet, ek het hierdie projek vandag oopgemaak met die idee dat ek verder aan my \"Forth\" gaan werk (alhoewel ek binnekort gaan ophou om dit Forth te noem, want dit is 'n stack-based programeeringstaal wat seker ge\x8bnspireer was deur Forth, maar toe ek meer kyk lyk dit my Forth is 'n spesifieke taal en ek het nie al die goed wat dit het nie, en ek wil potensie\x89l goed inbring wat nie deel is van Forth nie), ek glo my `rep` werk steeds nie behoorlik nie, maar toe sien ek my uncommitted veranderinge sit alles by hierdie \"pager\" komponent, so toe maak ek dit eers klaar. Ek het eintlik bietjie gesukkel om dit reg te implimenteer, maar toe vat ek die kode wat die teks in lyne in verwerk, en implimenteer dit in gewone C++, en toe kon ek beter ondersoek instel in wat presies gebeur totdat ek kon uitwerk waar my voute gel\x88 het. Maar nou ja, ek wil klaar kom met die Forth, dit op 'n goeie plek kry, en ek het 'n hele paar idees daarvoor, maar ek dink ook dis goed die dat ek 'n kort breek daarvan vat en aan iets anders werk, help om te vermy dat ek daar begin vassit en dan nie meer daaraan wil werk nie. Maar in elk geval, hoe veel het ek nou geskryf? Hopelik genoeg, want ek het nie juis vreeslik baie meer om oor te skryf op die oomblik nie.\n"
		"Dis goed! So sewehonderd woorde n\xa0 die Bybelaanhalings, en nog so driehonderd voor dit, so dan behoort ek veilig te wees."
	;

	auto pager = Pager(text);
	pager.draw();

	while (!pager.should_quit) {
		__asm__ volatile("hlt" ::: "memory");

		bool should_redraw = false;
		while (!ps2::events.empty()) {
			should_redraw = true;

			pager.handle_key(ps2::events.pop());
		}
		if (should_redraw) pager.draw();
	}
}

constexpr MenuEntry menu_entries[] = {
	{ "Snake game", snake::main },
	{ "Mieliepit interpreter (stack-based programming language)", mieliepit::main },
	{ "Display character map", character_map::main },
	{ "Text Editor", editor::main },
	{ "Uptime Tracker", uptime::main },
	{ "PEDMAS Calculator", calculator::main },
};
constexpr size_t menu_entries_len = sizeof(menu_entries)/sizeof(*menu_entries);
constexpr MenuEntry hidden_menu_entries[] = {
	{ "DEBUG: QueuedEventLoop Demo", queued_demo::main },
	{ "DEBUG: CallbackEventLoop Demo", callback_demo::main },
	{ "DEBUG: IgnoreEventLoop Demo", ignore_demo::main },
	{ "DEBUG: Pager Test", pager_test },
};
constexpr size_t hidden_menu_entries_len = sizeof(hidden_menu_entries)/sizeof(*hidden_menu_entries);

}

void main() {
	Menu menu(
		menu_entries, menu_entries_len,
		hidden_menu_entries, hidden_menu_entries_len,
		"AVAILABLE APPLICATIONS",
		"AVAILABLE APPLICATIONS - ADVANCED"
	);

	menu.draw();

	for (;;) {
		__asm__ volatile("hlt" ::: "memory");

		bool should_redraw = false;
		while (!ps2::events.empty())  {
			should_redraw = true;

			menu.handle_key(ps2::events.pop());
		}
		if (should_redraw) menu.draw();
	}
}

}
