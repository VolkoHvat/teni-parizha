#include "TPIdentitySubsystem.h"
#include "TeniParizha.h"

/* Таблицы перенесены из первой версии игры (legacy/teni_v1/game2/ident.c) без изменений текста. */
// именованное пространство имён: безопасно для unity-сборки
namespace TPIdentityData
{
// Порядок сословий совпадает с ETPSocialClass: Labor, Artisan, Shop, Soldier, Noble, Clergy, Vagrant, Servant, Scholar, Sailor
static const int32 CLN[10] = {22, 20, 14, 16, 12, 7, 10, 12, 7, 8};
static const float CLFEM[10] = {.5f, .35f, .5f, 0.f, .5f, .3f, .4f, .5f, .2f, 0.f};
static const int32 CLAGE[10][2] = {{18, 60}, {20, 62}, {25, 64}, {19, 46}, {22, 70}, {28, 72}, {17, 72}, {15, 58}, {19, 60}, {18, 55}};
static const TCHAR* CLNAME[10]={TEXT("Рабочий люд"),TEXT("Ремесленники"),TEXT("Торговцы"),TEXT("Солдаты"),TEXT("Дворянство"),TEXT("Духовенство"),TEXT("Бродяги"),TEXT("Прислуга"),TEXT("Учёный люд"),TEXT("Речники")};
static const TCHAR* NM_M[40]={TEXT("Жан"),TEXT("Пьер"),TEXT("Луи"),TEXT("Антуан"),TEXT("Франсуа"),TEXT("Жак"),TEXT("Мишель"),TEXT("Андре"),TEXT("Бернар"),TEXT("Клод"),TEXT("Этьен"),TEXT("Гийом"),TEXT("Матьё"),TEXT("Никола"),TEXT("Огюстен"),TEXT("Рене"),TEXT("Тома"),TEXT("Венсан"),TEXT("Лоран"),TEXT("Люсьен"),TEXT("Морис"),TEXT("Дени"),TEXT("Поль"),TEXT("Реми"),TEXT("Гаспар"),TEXT("Лазар"),TEXT("Ксавье"),TEXT("Ив"),TEXT("Фабьен"),TEXT("Шарль"),TEXT("Анри"),TEXT("Эжен"),TEXT("Жером"),TEXT("Максим"),TEXT("Сильвен"),TEXT("Ноэль"),TEXT("Янн"),TEXT("Тьерри"),TEXT("Давид"),TEXT("Арман")};
static const TCHAR* NM_F[40]={TEXT("Мари"),TEXT("Анна"),TEXT("Жанна"),TEXT("Мадлен"),TEXT("Катрин"),TEXT("Сюзанна"),TEXT("Маргарита"),TEXT("Луиза"),TEXT("Элоиза"),TEXT("Жюли"),TEXT("Софи"),TEXT("Изабелла"),TEXT("Франсуаза"),TEXT("Клотильда"),TEXT("Адель"),TEXT("Бланш"),TEXT("Шарлотта"),TEXT("Камилла"),TEXT("Эмили"),TEXT("Эстель"),TEXT("Жозефина"),TEXT("Аделина"),TEXT("Виолетта"),TEXT("Эжени"),TEXT("Ортанс"),TEXT("Берта"),TEXT("Мартина"),TEXT("Тереза"),TEXT("Элен"),TEXT("Колетт"),TEXT("Бриджит"),TEXT("Николь"),TEXT("Габриэль"),TEXT("Эдме"),TEXT("Роза"),TEXT("Сесиль"),TEXT("Леони"),TEXT("Манон"),TEXT("Фанни"),TEXT("Люси")};
static const TCHAR* SN[60]={TEXT("Дюпон"),TEXT("Леруа"),TEXT("Мартен"),TEXT("Бернар"),TEXT("Дюбуа"),TEXT("Морель"),TEXT("Лефевр"),TEXT("Руссо"),TEXT("Гарнье"),TEXT("Фонтен"),TEXT("Клеман"),TEXT("Мерсье"),TEXT("Блан"),TEXT("Шевалье"),TEXT("Пети"),TEXT("Лорен"),TEXT("Дюран"),TEXT("Мишо"),TEXT("Рено"),TEXT("Бретон"),TEXT("Леблан"),TEXT("Моро"),TEXT("Симон"),TEXT("Лакруа"),TEXT("Берже"),TEXT("Ланглуа"),TEXT("Готье"),TEXT("Перье"),TEXT("Маршан"),TEXT("Вернье"),TEXT("Боннет"),TEXT("Фурнье"),TEXT("Бушар"),TEXT("Ренар"),TEXT("Колен"),TEXT("Бодуэн"),TEXT("Эрве"),TEXT("Ламбер"),TEXT("Тиссо"),TEXT("Вуазен"),TEXT("Каде"),TEXT("Жиро"),TEXT("Пайен"),TEXT("Робер"),TEXT("Адан"),TEXT("Мариэ"),TEXT("Шове"),TEXT("Дени"),TEXT("Тессье"),TEXT("Варен"),TEXT("Барбье"),TEXT("Лежандр"),TEXT("Менье"),TEXT("Бюиссон"),TEXT("Куртуа"),TEXT("Шарпантье"),TEXT("Лемуан"),TEXT("Ляшез"),TEXT("Пуарье"),TEXT("Башелье")};
static const TCHAR* NSN[16]={TEXT("де Лаваль"),TEXT("де Бофор"),TEXT("де Сен-Клер"),TEXT("де Ла Тур"),TEXT("де Верней"),TEXT("де Шатийон"),TEXT("де Роган"),TEXT("де Грамон"),TEXT("де Бельвиль"),TEXT("де Мортемар"),TEXT("де Люинь"),TEXT("де Ла Фер"),TEXT("де Монтескье"),TEXT("де Крийон"),TEXT("де Лорж"),TEXT("де Ноайль")};
static const TCHAR* NT_M[5]={TEXT("Маркиз"),TEXT("Граф"),TEXT("Барон"),TEXT("Виконт"),TEXT("Шевалье")},* NT_F[5]={TEXT("Маркиза"),TEXT("Графиня"),TEXT("Баронесса"),TEXT("Виконтесса"),TEXT("Мадемуазель")};
static const TCHAR* JM[10][9]={{TEXT("носильщик"),TEXT("чернорабочий"),TEXT("водонос"),TEXT("возчик"),TEXT("дровосек"),TEXT("угольщик"),TEXT("каменщик"),TEXT("подёнщик"),TEXT("трубочист")},{TEXT("пекарь"),TEXT("сапожник"),TEXT("портной"),TEXT("кузнец"),TEXT("плотник"),TEXT("часовщик"),TEXT("ювелир"),TEXT("переплётчик"),TEXT("бондарь")},{TEXT("мясник"),TEXT("книготорговец"),TEXT("аптекарь"),TEXT("бакалейщик"),TEXT("виноторговец"),TEXT("торговец тканями"),TEXT("цирюльник"),TEXT("торговец углём"),TEXT("зеленщик")},{TEXT("гвардеец"),TEXT("сержант"),TEXT("капрал"),TEXT("гренадёр"),TEXT("лейтенант стражи"),TEXT("барабанщик"),TEXT("гвардеец"),TEXT("гвардеец"),TEXT("капрал")},{TEXT("маркиз"),TEXT("граф"),TEXT("барон"),TEXT("виконт"),TEXT("шевалье"),TEXT("маркиз"),TEXT("граф"),TEXT("барон"),TEXT("виконт")},{TEXT("священник"),TEXT("дьякон"),TEXT("органист"),TEXT("пономарь"),TEXT("монах-францисканец"),TEXT("священник"),TEXT("дьякон"),TEXT("органист"),TEXT("пономарь")},{TEXT("бродяга"),TEXT("нищий"),TEXT("уличный музыкант"),TEXT("бывший солдат"),TEXT("карманник"),TEXT("бродяга"),TEXT("нищий"),TEXT("уличный музыкант"),TEXT("бродяга")},{TEXT("лакей"),TEXT("кучер"),TEXT("камердинер"),TEXT("дворецкий"),TEXT("курьер"),TEXT("подмастерье"),TEXT("лакей"),TEXT("кучер"),TEXT("курьер")},{TEXT("студент"),TEXT("писарь"),TEXT("учитель"),TEXT("адвокат"),TEXT("врач"),TEXT("печатник"),TEXT("публицист"),TEXT("студент"),TEXT("писарь")},{TEXT("лодочник"),TEXT("речной грузчик"),TEXT("матрос"),TEXT("рыбак"),TEXT("паромщик"),TEXT("канатчик"),TEXT("лодочник"),TEXT("рыбак"),TEXT("матрос")}};
static const TCHAR* JF[10][8]={{TEXT("прачка"),TEXT("молочница"),TEXT("подёнщица"),TEXT("торговка зеленью"),TEXT("огородница"),TEXT("уборщица"),TEXT("швея-подёнщица"),TEXT("прачка")},{TEXT("кружевница"),TEXT("шляпница"),TEXT("вышивальщица"),TEXT("свечница"),TEXT("портниха"),TEXT("корзинщица"),TEXT("кружевница"),TEXT("шляпница")},{TEXT("цветочница"),TEXT("торговка рыбой"),TEXT("лавочница"),TEXT("булочница"),TEXT("торговка фруктами"),TEXT("хозяйка кофейни"),TEXT("цветочница"),TEXT("лавочница")},{TEXT("гвардеец"),TEXT("гвардеец"),TEXT("гвардеец"),TEXT("гвардеец"),TEXT("гвардеец"),TEXT("гвардеец"),TEXT("гвардеец"),TEXT("гвардеец")},{TEXT("маркиза"),TEXT("графиня"),TEXT("баронесса"),TEXT("виконтесса"),TEXT("придворная дама"),TEXT("маркиза"),TEXT("графиня"),TEXT("баронесса")},{TEXT("монахиня"),TEXT("настоятельница"),TEXT("сестра милосердия"),TEXT("монахиня"),TEXT("монахиня"),TEXT("настоятельница"),TEXT("сестра милосердия"),TEXT("монахиня")},{TEXT("нищенка"),TEXT("гадалка"),TEXT("уличная певица"),TEXT("попрошайка"),TEXT("нищенка"),TEXT("гадалка"),TEXT("уличная певица"),TEXT("попрошайка")},{TEXT("горничная"),TEXT("кухарка"),TEXT("нянюшка"),TEXT("служанка"),TEXT("горничная"),TEXT("кухарка"),TEXT("нянюшка"),TEXT("служанка")},{TEXT("учительница"),TEXT("переписчица"),TEXT("сиделка"),TEXT("переводчица"),TEXT("учительница"),TEXT("переписчица"),TEXT("сиделка"),TEXT("переводчица")},{TEXT("рыбачка"),TEXT("рыбачка"),TEXT("рыбачка"),TEXT("рыбачка"),TEXT("рыбачка"),TEXT("рыбачка"),TEXT("рыбачка"),TEXT("рыбачка")}};
static const TCHAR* TRAIT[24]={TEXT("Смелость"),TEXT("Осторожность"),TEXT("Болтливость"),TEXT("Угрюмость"),TEXT("Добродушие"),TEXT("Вспыльчивость"),TEXT("Набожность"),TEXT("Суеверность"),TEXT("Расчётливость"),TEXT("Мечтательность"),TEXT("Честность"),TEXT("Хитрость"),TEXT("Щедрость"),TEXT("Скупость"),TEXT("Любопытство"),TEXT("Гордость"),TEXT("Застенчивость"),TEXT("Весёлый нрав"),TEXT("Ворчливость"),TEXT("Преданность"),TEXT("Упрямство"),TEXT("Находчивость"),TEXT("Трудолюбие"),TEXT("Лень")};
static const TCHAR* QUIRK[24]={TEXT("Никогда не отступает перед стражей."),TEXT("Трижды проверяет замок перед сном."),TEXT("Не умеет молчать дольше минуты."),TEXT("Здоровается, лишь когда с ним заговорят первым."),TEXT("Угощает прохожих, чем бог послал."),TEXT("Вспыхивает из-за пустяка и тут же остывает."),TEXT("Каждое утро заходит в церковь помолиться."),TEXT("Не выйдет из дома, если чёрная кошка перебежала дорогу."),TEXT("Записывает каждую потраченную монету."),TEXT("Подолгу смотрит на крыши и мечтает о другой жизни."),TEXT("Не берёт ни су сверх положенного."),TEXT("Умеет выкрутиться из любой беды."),TEXT("Раздаёт последнее тем, кому хуже."),TEXT("Пересчитывает сдачу трижды."),TEXT("Заглядывает в чужие окна и слушает чужие разговоры."),TEXT("Не склоняет головы ни перед кем."),TEXT("Краснеет, стоит с ним заговорить."),TEXT("Смеётся громче всех в квартале."),TEXT("Ворчит на погоду, цены и соседей."),TEXT("Хранит верность старым друзьям до конца."),TEXT("Если что решил, то не свернёт."),TEXT("Находит выход там, где другие сдаются."),TEXT("Встаёт раньше петухов и ложится позже всех."),TEXT("Спит днём и жалуется на жизнь вечером.")};
static const TCHAR* ORIG[15]={TEXT("в Руане"),TEXT("в Лионе"),TEXT("в Бордо"),TEXT("в Марселе"),TEXT("в Нанте"),TEXT("в Страсбурге"),TEXT("в Орлеане"),TEXT("в деревне под Шартром"),TEXT("в предместье Сен-Антуан"),TEXT("в Нормандии"),TEXT("в Бретани"),TEXT("в Пикардии"),TEXT("в Лотарингии"),TEXT("в Оверни"),TEXT("в Париже, на соседней улице")};
static const TCHAR* LIFE[10][4]={{TEXT("С рассвета до заката таскает грузы на набережной ради куска хлеба."),TEXT("Берётся за любую работу: сегодня на рынке, завтра на стройке."),TEXT("Привёз в Париж телегу дров и так и осел в квартале."),TEXT("Живёт в мансарде под самой крышей и знает каждый карниз.")},{TEXT("Держит маленькую мастерскую на соседней улице и гордится своей работой."),TEXT("Учит подмастерье ремеслу и жалуется на дороговизну материалов."),TEXT("Делает вещи, которые покупают даже придворные, но платят с опозданием."),TEXT("Работает по ночам при свече, чтобы успеть к сроку.")},{TEXT("Торгует с утра до вечера и знает цену каждому слуху."),TEXT("Откладывает по монете в старый чулок на чёрный день."),TEXT("Лавка досталась по наследству и связывает с прошлым."),TEXT("Все новости квартала сначала попадают к прилавку.")},{TEXT("Несёт караул и мечтает о пенсии и тихой ферме."),TEXT("Строг с горожанами, но втайне сочувствует голодным."),TEXT("Служит Короне, хотя жалованье задерживают уже третий месяц."),TEXT("Гордится мундиром и начищает сапоги до зеркального блеска.")},{TEXT("Живёт в роскошном особняке, но средства тают быстрее, чем кажется."),TEXT("Устраивает салоны и любит слушать чужие секреты."),TEXT("Тайно читает запрещённые книги философов."),TEXT("Боится перемен и не доверяет ни слугам, ни друзьям.")},{TEXT("Служит в приходе и слушает исповеди половины квартала."),TEXT("Раздаёт хлеб беднякам, хотя церкви самой едва хватает."),TEXT("Играет по вечерам на старом органе, когда никто не слышит."),TEXT("Спорит с епископом о бедности и о том, кому служит церковь.")},{TEXT("Ночует где придётся: то под мостом, то в сарае у рынка."),TEXT("Знает каждый закоулок и каждую щель в заборе."),TEXT("Просит милостыню у собора и знает всех прихожан в лицо."),TEXT("Когда-то жилось лучше, но прошлое лучше не вспоминать.")},{TEXT("Прислуживает в богатом доме и видит то, что хозяевам лучше скрыть."),TEXT("Бегает по поручениям с утра до ночи и знает всех торговцев."),TEXT("Мечтает накопить на лавку и уйти от хозяев."),TEXT("Тайком читает хозяйские книги, когда никого нет дома.")},{TEXT("Спорит о правах человека в кофейне до закрытия."),TEXT("Переписывает бумаги, а по ночам пишет собственные памфлеты."),TEXT("Лечит бедных бесплатно, потому что так велит совесть."),TEXT("Учит детей читать и верит, что грамота спасёт мир.")},{TEXT("Возит грузы по Сене и знает каждую мель."),TEXT("Перевозит людей с берега на берег и слышит чужие разговоры."),TEXT("Ловит рыбу на рассвете и продаёт улов у моста."),TEXT("Копит на собственную лодку и бережёт каждую монету.")}};
static const TCHAR* WORRY[10]={TEXT("Беспокоится из-за дорогого хлеба."),TEXT("Слышал{|а}, что в Версале собрались Генеральные штаты, и ждёт перемен."),TEXT("Боится, что город заполнят голодные толпы."),TEXT("Много слышит о бунтах и ночами не спит."),TEXT("Надеется, что лето будет урожайным."),TEXT("Не доверяет слухам, но всё равно их пересказывает."),TEXT("Опасается новых налогов и сборщиков."),TEXT("Подолгу слушает ораторов на площади."),TEXT("Верит, что король справедлив, а лукавят советники."),TEXT("Мечтает уехать из шумного Парижа.")};
static const TCHAR* SECRET[10]={TEXT("Тайно хранит письма от человека, которому не следует писать."),TEXT("Прячет под половицей мешочек золота."),TEXT("Однажды увидел{|а} в подвале то, о чём лучше молчать."),TEXT("Знает, где спрятан склад оружия, и боится об этом думать."),TEXT("Давно ищет пропавшего брата."),TEXT("Тайно помогает беглым крестьянам."),TEXT("Не умеет читать, но скрывает это от всех."),TEXT("Носит на шее медальон с портретом, о котором никому не говорит."),TEXT("Долж{ен|на} крупную сумму ростовщику."),TEXT("Втайне пишет стихи.")};
static const TCHAR* RUMOR[10][3]={{TEXT("Говорят, хлеб подорожает к осени."),TEXT("На набережной ищут грузчиков, платят хорошо."),TEXT("Не лезь на крыши в дождь: черепица скользкая.")},{TEXT("Для хорошей работы нужны время и терпение."),TEXT("Заказы идут туго, все ждут перемен."),TEXT("Ночью на мастерских шалят воры.")},{TEXT("Торговля стоит: люди боятся тратить деньги."),TEXT("Лучшие вести узнаёшь у прилавка."),TEXT("Стража проверяет лавки по утрам.")},{TEXT("Не носись по улицам, стража такого не любит."),TEXT("В саду маркиза выставили усиленный пост."),TEXT("Порядок для всех один.")},{TEXT("В салонах только и говорят что о Версале."),TEXT("Мир скоро изменится, и не в нашу пользу."),TEXT("Не стоит верить газетам.")},{TEXT("Молитесь о хлебе и о мире."),TEXT("Колокольню видно на весь квартал."),TEXT("Двери церкви открыты для всех.")},{TEXT("Дай монету, и я расскажу, где пройти по крышам."),TEXT("Стража гоняет нас с площади."),TEXT("Лучшие сны бывают на сеновале.")},{TEXT("Мои хозяева скоро уедут из города."),TEXT("Кучера знают все улицы, но не все крыши."),TEXT("Лучше не слышать, о чём говорят за столом.")},{TEXT("Скоро книги будут читать все."),TEXT("Я пишу памфлет о правах человека."),TEXT("Свобода начинается с грамоты.")},{TEXT("Река сегодня высокая."),TEXT("Мосты держат крепче, чем кажется."),TEXT("На острове голуби, а собор в тумане.")}};
static const TCHAR* RUN_NICK[5]={TEXT("Жак-Трубочист"),TEXT("Лиза-Прачка"),TEXT("Студент Эмиль"),TEXT("Тень"),TEXT("Ворон")};
static const TCHAR* RUNNER_NAME[5] = {TEXT("Жак Бодуэн"), TEXT("Лиза Ферран"), TEXT("Эмиль Вернье"), TEXT("Сесиль Дюран"), TEXT("Арман Шове")};
static const TCHAR* RUNNER_JOB[5] = {TEXT("трубочист, бегун Лиги крыш"), TEXT("прачка, бегунья Лиги крыш"), TEXT("студент-медик, бегун Лиги крыш"), TEXT("горничная, бегунья Лиги крыш"), TEXT("бывший трубочист, чемпион Лиги крыш")};
static const TCHAR* RUNNER_BIO[5] = {
	TEXT("Вырос на крышах: с восьми лет чистил дымоходы и знает каждую трубу квартала. Весёлый и шумный, первым встречает новичков Лиги."),
	TEXT("Носит корзины белья с набережной на мансарды и давно поняла, что по крышам быстрее, чем по лестницам. Упряма и не терпит поражений."),
	TEXT("Сбегает с лекций, чтобы бегать по карнизам. Считает прыжки по формулам и уверен, что однажды опишет паркур в трактате."),
	TEXT("Днём прислуживает в особняке, ночью исчезает по крышам. Говорит мало, двигается бесшумно, за что и получила прозвище."),
	TEXT("Бывший трубочист, а ныне чемпион Лиги крыш. Ни разу не проигрывал гонку к собору. Насмешлив и горд, но уважает тех, кто не сдаётся.")};

static FColor LerpColor(const FColor& A, const FColor& B, float T)
{
	T = FMath::Clamp(T, 0.f, 1.f);
	return FColor((uint8)FMath::RoundToInt(A.R + (B.R - A.R) * T), (uint8)FMath::RoundToInt(A.G + (B.G - A.G) * T), (uint8)FMath::RoundToInt(A.B + (B.B - A.B) * T));
}

/** «Родил{ся|ась}» → «Родился» или «Родилась» */
static FString GenderForm(const FString& Src, bool bFemale)
{
	FString Out;
	Out.Reserve(Src.Len());
	int32 I = 0;
	while (I < Src.Len())
	{
		if (Src[I] == TEXT('{'))
		{
			const int32 Bar = Src.Find(TEXT("|"), ESearchCase::CaseSensitive, ESearchDir::FromStart, I);
			const int32 End = Bar >= 0 ? Src.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Bar) : -1;
			if (Bar < 0 || End < 0) { Out.AppendChar(Src[I]); ++I; continue; }
			Out += bFemale ? Src.Mid(Bar + 1, End - Bar - 1) : Src.Mid(I + 1, Bar - I - 1);
			I = End + 1;
		}
		else { Out.AppendChar(Src[I]); ++I; }
	}
	return Out;
}
} // namespace TPIdentityData

uint32 UTPIdentitySubsystem::Hash(int32 A, int32 B)
{
	uint32 H = (uint32)A * 374761393u + (uint32)B * 668265263u;
	H = (H ^ (H >> 13)) * 1274126177u;
	return H ^ (H >> 16);
}

void UTPIdentitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Generate();
	AssignRoles();
	FinishIdentities();
	UE_LOG(LogTeni, Log, TEXT("Жители квартала: %d, художник %d, карманник %d"), Citizens.Num(), PainterId, ThiefId);
}

void UTPIdentitySubsystem::Generate()
{
	using namespace TPIdentityData;
	Citizens.SetNum(TPCitizens::Num);
	int32 K = 0;
	for (int32 C = 0; C < 10; ++C)
	{
		for (int32 J = 0; J < CLN[C]; ++J, ++K)
		{
			FTPIdentity& D = Citizens[K];
			D = FTPIdentity();
			const uint32 H = Hash(K * 7 + 13, 3);
			D.Class = (ETPSocialClass)C;
			D.bFemale = ((H >> 3) % 1000u) < (uint32)(CLFEM[C] * 1000.f);
			if (C == (int32)ETPSocialClass::Clergy) D.bFemale = (J == 2 || J == 5);
			if (C == (int32)ETPSocialClass::Noble) D.bFemale = (J & 1) != 0;
			D.Age = CLAGE[C][0] + (int32)((H >> 9) % (uint32)(CLAGE[C][1] - CLAGE[C][0] + 1));

			const int32 F = (K * 7 + 3) % 40;
			int32 L = (K * 13 + 5) % 60;
			const TCHAR* FN = D.bFemale ? NM_F[F] : NM_M[F];
			for (int32 Tries = 0; Tries < 60; ++Tries)
			{
				if (C == (int32)ETPSocialClass::Noble)
				{
					const int32 T = (int32)((H >> 5) % 5u);
					D.Name = FString::Printf(TEXT("%s %s %s"), D.bFemale ? NT_F[T] : NT_M[T], FN, NSN[(K * 5 + J + Tries) % 16]);
				}
				else if (C == (int32)ETPSocialClass::Clergy)
					D.Name = FString::Printf(TEXT("%s %s %s"), D.bFemale ? TEXT("Сестра") : TEXT("Отец"), FN, SN[L]);
				else
					D.Name = FString::Printf(TEXT("%s %s"), FN, SN[L]);
				bool bDup = false;
				for (int32 Q = 0; Q < K; ++Q)
					if (Citizens[Q].Name.Equals(D.Name, ESearchCase::CaseSensitive)) { bDup = true; break; }
				if (!bDup) break;
				L = (L + 7) % 60;
			}
			D.Job = D.bFemale ? JF[C][J % 8] : JM[C][J % 9];
			D.Trait1Index = (int32)(Hash(K, 577) % 24u);
			D.Trait2Index = (int32)(Hash(K, 991) % 24u);
			if (D.Trait2Index == D.Trait1Index || D.Trait2Index == (D.Trait1Index ^ 1)) D.Trait2Index = (D.Trait1Index + 6) % 24;
		}
	}
}

void UTPIdentitySubsystem::AssignRoles()
{
	using namespace TPIdentityData;
	const int32 N = Citizens.Num();
	// художник — пожилой ремесленник
	PainterId = -1;
	for (int32 I = 0; I < N; ++I)
		if (Citizens[I].Class == ETPSocialClass::Artisan && !Citizens[I].bFemale && Citizens[I].Age > 40) { PainterId = I; break; }
	if (PainterId < 0) PainterId = CLN[0];
	{
		FTPIdentity& P = Citizens[PainterId];
		P.Name = TEXT("Мэтр Огюстен Брюн");
		P.Job = TEXT("художник-портретист");
		P.Role = ETPCitizenRole::Painter;
		P.Age = 58; P.Trait1Index = 14; P.Trait2Index = 12;
	}
	// бегуны Лиги крыш
	static const ETPSocialClass Want[5] = {ETPSocialClass::Labor, ETPSocialClass::Labor, ETPSocialClass::Scholar, ETPSocialClass::Servant, ETPSocialClass::Vagrant};
	static const bool WantFemale[5] = {false, true, false, true, false};
	RunnerIds.Init(-1, TPCitizens::NumRunners);
	for (int32 R = 0; R < TPCitizens::NumRunners; ++R)
	{
		for (int32 I = 0; I < N; ++I)
		{
			const FTPIdentity& D = Citizens[I];
			if (D.Role != ETPCitizenRole::None || D.Class != Want[R] || D.bFemale != WantFemale[R] || D.Age > 34) continue;
			RunnerIds[R] = I; break;
		}
		if (RunnerIds[R] < 0)
			for (int32 I = 0; I < N; ++I)
				if (Citizens[I].Role == ETPCitizenRole::None && Citizens[I].Class == Want[R]) { RunnerIds[R] = I; break; }
		if (RunnerIds[R] < 0)
			for (int32 I = 0; I < N; ++I)
				if (Citizens[I].Role == ETPCitizenRole::None) { RunnerIds[R] = I; break; }
		FTPIdentity& D = Citizens[RunnerIds[R]];
		D.Role = ETPCitizenRole::Runner;
		D.RunnerIndex = R;
		D.bFemale = WantFemale[R];
		D.Nick = RUN_NICK[R];
		if (D.Age > 30) D.Age = 19 + R * 2;
		D.Name = RUNNER_NAME[R];
	}
	// однофамильцы особых персонажей становятся «младшими»
	for (int32 I = 0; I < N; ++I)
	{
		FTPIdentity& D = Citizens[I];
		if (D.Role == ETPCitizenRole::Runner || D.Role == ETPCitizenRole::Painter) continue;
		for (int32 R = 0; R < TPCitizens::NumRunners + 1; ++R)
		{
			const FString& Other = R < TPCitizens::NumRunners ? Citizens[RunnerIds[R]].Name : Citizens[PainterId].Name;
			if (D.Name.Equals(Other, ESearchCase::CaseSensitive))
				D.Name += D.bFemale ? TEXT("-младшая") : TEXT("-младший");
		}
	}
	for (int32 R = 0; R < TPCitizens::NumRunners; ++R) Citizens[RunnerIds[R]].Job = RUNNER_JOB[R];
	// карманник
	ThiefId = -1;
	for (int32 I = 0; I < N; ++I)
		if (Citizens[I].Role == ETPCitizenRole::None && Citizens[I].Class == ETPSocialClass::Vagrant && !Citizens[I].bFemale) { ThiefId = I; break; }
	if (ThiefId >= 0)
	{
		Citizens[ThiefId].Role = ETPCitizenRole::Thief;
		Citizens[ThiefId].Job = TEXT("карманник");
		Citizens[ThiefId].Age = 24;
	}
}

void UTPIdentitySubsystem::FinishIdentities()
{
	using namespace TPIdentityData;
	for (int32 K = 0; K < Citizens.Num(); ++K)
	{
		FTPIdentity& D = Citizens[K];
		const int32 C = (int32)D.Class;
		FString Tmp;
		if (D.Role == ETPCitizenRole::Painter)
			Tmp = TEXT("Родился в Лионе. Тридцать лет пишет портреты: сначала для знати, теперь для всех, кто согласится посидеть смирно. Задумал «Лица квартала» — сто двадцать восемь портретов всех соседей, от маркиза до нищего. Любопытен, как ребёнок, и щедр до последнего холста.");
		else if (D.Role == ETPCitizenRole::Runner)
			Tmp = FString::Printf(TEXT("Прозвище — «%s». %s %s"), RUN_NICK[D.RunnerIndex], RUNNER_BIO[D.RunnerIndex], QUIRK[D.Trait1Index]);
		else if (D.Role == ETPCitizenRole::Thief)
			Tmp = FString::Printf(TEXT("Родился в предместье Сен-Антуан. Ловок на руку, но не на ноги: стоит его окликнуть, как он бросается бежать по крышам. %s"), QUIRK[D.Trait1Index]);
		else
		{
			const uint32 G = Hash(K, 1913);
			Tmp = FString::Printf(TEXT("Родил{ся|ась} %s. %s %s %s"), ORIG[(G >> 3) % 15u], LIFE[C][(G >> 9) % 4u], WORRY[(G >> 13) % 10u], QUIRK[D.Trait1Index]);
			if ((G >> 20) % 3u == 0) { Tmp += TEXT(" "); Tmp += SECRET[(G >> 23) % 10u]; }
		}
		D.Bio = GenderForm(Tmp, D.bFemale);
		D.Trait1 = TRAIT[D.Trait1Index];
		D.Trait2 = TRAIT[D.Trait2Index];
		MakeLook(D, K);
		if (D.Role == ETPCitizenRole::Runner)
		{
			D.Coat = D.RunnerIndex == 4 ? FColor(24, 22, 28) : LerpColor(D.Coat, FColor(60, 60, 70), .3f);
			D.Trim = FColor(200, 40, 48);
		}
		if (D.Role == ETPCitizenRole::Painter) { D.Coat = FColor(110, 70, 90); D.Hair = FColor(214, 210, 204); }
	}
}

void UTPIdentitySubsystem::MakeLook(FTPIdentity& D, int32 K) const
{
	using namespace TPIdentityData;
	const uint32 H = Hash(K, 71);
	static const FColor SK[6] = {FColor(240, 206, 176), FColor(230, 190, 160), FColor(216, 172, 140), FColor(196, 148, 116), FColor(170, 120, 92), FColor(236, 200, 184)};
	static const FColor HR[6] = {FColor(50, 40, 40), FColor(90, 64, 44), FColor(140, 104, 64), FColor(30, 28, 30), FColor(176, 128, 70), FColor(120, 56, 44)};
	static const FColor MC[10][4] = {
		{FColor(112, 92, 66), FColor(90, 84, 70), FColor(120, 104, 84), FColor(100, 90, 60)},
		{FColor(60, 80, 110), FColor(80, 100, 70), FColor(120, 80, 60), FColor(90, 70, 60)},
		{FColor(130, 60, 60), FColor(160, 120, 60), FColor(70, 90, 110), FColor(90, 60, 90)},
		{FColor(52, 76, 140), FColor(52, 76, 140), FColor(52, 76, 140), FColor(52, 76, 140)},
		{FColor(150, 60, 90), FColor(60, 80, 150), FColor(210, 180, 70), FColor(100, 140, 120)},
		{FColor(30, 30, 34), FColor(30, 30, 34), FColor(36, 34, 40), FColor(30, 30, 34)},
		{FColor(90, 84, 76), FColor(100, 92, 80), FColor(80, 76, 70), FColor(96, 86, 70)},
		{FColor(60, 90, 70), FColor(110, 50, 50), FColor(50, 60, 90), FColor(80, 70, 60)},
		{FColor(40, 44, 60), FColor(50, 50, 56), FColor(60, 54, 50), FColor(44, 52, 60)},
		{FColor(50, 70, 100), FColor(60, 80, 90), FColor(80, 70, 56), FColor(70, 90, 100)}};
	static const FColor WC[10][4] = {
		{FColor(130, 100, 80), FColor(100, 110, 120), FColor(120, 110, 90), FColor(140, 110, 90)},
		{FColor(120, 80, 100), FColor(90, 110, 130), FColor(130, 120, 80), FColor(100, 100, 110)},
		{FColor(150, 90, 100), FColor(90, 120, 140), FColor(160, 130, 70), FColor(120, 80, 130)},
		{FColor(52, 76, 140), FColor(52, 76, 140), FColor(52, 76, 140), FColor(52, 76, 140)},
		{FColor(180, 70, 110), FColor(80, 110, 190), FColor(220, 190, 80), FColor(110, 170, 150)},
		{FColor(28, 28, 32), FColor(28, 28, 32), FColor(34, 32, 38), FColor(28, 28, 32)},
		{FColor(96, 88, 80), FColor(86, 80, 74), FColor(104, 92, 76), FColor(90, 84, 70)},
		{FColor(40, 40, 46), FColor(50, 50, 56), FColor(36, 44, 40), FColor(46, 40, 50)},
		{FColor(70, 70, 90), FColor(80, 70, 80), FColor(60, 80, 90), FColor(90, 80, 70)},
		{FColor(80, 100, 120), FColor(100, 90, 80), FColor(90, 110, 100), FColor(110, 100, 90)}};
	const int32 C = (int32)D.Class;
	D.Skin = SK[H % 6u];
	if (D.Class == ETPSocialClass::Vagrant) D.Skin = LerpColor(D.Skin, FColor(120, 90, 70), .3f);
	D.Hair = HR[(H >> 4) % 6u];
	if (D.Age > 56) D.Hair = (H & 1u) ? FColor(214, 210, 204) : FColor(176, 172, 168);
	else if (D.Age > 44 && (H & 2u)) D.Hair = LerpColor(D.Hair, FColor(190, 186, 180), .5f);
	D.Trim = FColor(240, 236, 226);
	const int32 CI = (int32)((H >> 12) % 4u);
	D.Coat = D.bFemale ? WC[C][CI] : MC[C][CI];
	D.Pants = D.bFemale ? LerpColor(D.Coat, FColor(60, 50, 56), .25f) : FColor((uint8)(70 + (H >> 3) % 20u), 64, 60);
	switch (D.Class)
	{
	case ETPSocialClass::Soldier: D.Pants = FColor(236, 236, 240); D.Trim = FColor(190, 40, 50); break;
	case ETPSocialClass::Noble: D.Hair = FColor(236, 232, 226); D.Trim = FColor(214, 176, 70); if (!D.bFemale) D.Pants = FColor(236, 232, 220); break;
	case ETPSocialClass::Clergy: D.Trim = FColor(250, 248, 248); D.Pants = D.Coat; break;
	case ETPSocialClass::Vagrant: D.Pants = FColor(86, 80, 72); break;
	case ETPSocialClass::Servant: if (D.bFemale) D.Trim = FColor(250, 248, 248); break;
	default: break;
	}
	// рост: женщины чуть ниже, у каждого своя поправка
	D.Height = (D.bFemale ? 0.95f : 1.f) + ((int32)((H >> 16) % 9u) - 4) * 0.01f - (D.Age > 60 ? 0.02f : 0.f);
}

const FTPIdentity& UTPIdentitySubsystem::Get(int32 Index) const
{
	static const FTPIdentity Empty;
	return Citizens.IsValidIndex(Index) ? Citizens[Index] : Empty;
}

FTPIdentity UTPIdentitySubsystem::GetIdentity(int32 Index) const { return Get(Index); }

FString UTPIdentitySubsystem::GetRumor(int32 Index, int32 Salt) const
{
	using namespace TPIdentityData;
	if (!Citizens.IsValidIndex(Index)) return FString();
	return RUMOR[(int32)Citizens[Index].Class][Hash(Index, Salt) % 3u];
}

FString UTPIdentitySubsystem::ClassName(ETPSocialClass Class)
{
	using namespace TPIdentityData;
	const int32 C = FMath::Clamp((int32)Class, 0, 9);
	return CLNAME[C];
}

FLinearColor UTPIdentitySubsystem::ClassColor(ETPSocialClass Class)
{
	static const FColor CLCOL[10] = {FColor(150, 120, 90), FColor(90, 120, 160), FColor(170, 90, 90), FColor(70, 100, 180), FColor(200, 170, 80),
		FColor(110, 100, 120), FColor(120, 110, 100), FColor(90, 140, 110), FColor(130, 110, 170), FColor(80, 150, 170)};
	return FLinearColor(CLCOL[FMath::Clamp((int32)Class, 0, 9)]);
}

FString UTPIdentitySubsystem::RunnerNick(int32 RunnerIndex)
{
	using namespace TPIdentityData;
	return (RunnerIndex >= 0 && RunnerIndex < 5) ? FString(RUN_NICK[RunnerIndex]) : FString();
}
