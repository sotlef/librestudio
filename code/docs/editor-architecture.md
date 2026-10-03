# Редактор Code: разбор архитектуры Scintilla 5.6.7 и наш план

Scintilla используется только как **справочник требований**: в LibreStudio она не
тянется ни как зависимость, ни как исходный код. Лексеры (Lexilla) в архив не
входят и нам не нужны: подсветку даёт сервис анализа.

## 1. Как устроена Scintilla (src/, ~33k строк)

Три слоя (doc/Design.html): портируемая библиотека → ядро → платформа
(win32/gtk/qt/cocoa). Ядро не знает про окна и рисует через `Surface`/`Window`.

### 1.1 Модель (без UI) — переносим почти целиком
| Класс | Что делает | Ключевая идея |
|---|---|---|
| `SplitVector<T>` | буфер с «зазором» | вставка/удаление у курсора O(1), сдвиг зазора только при прыжке |
| `Partitioning` | позиции начал строк | те же gap-технологии: вставка текста сдвигает **один шаг** (`stepPartition`), а не все строки |
| `CellBuffer` | текст + стили + строки | `substance` (байты), `style` (по байту на символ), `LineVector`; все изменения — `Action` в `UndoHistory`; отслеживает CRLF-склейки |
| `UndoHistory` | undo/redo | группы действий, точка сохранения (`savePoint`), `tentative` (IME), слияние набора символов |
| `ChangeHistory` | маркеры «изменено/сохранено/откатано» | полоска слева от строк как в VS |
| `RunStyles<D,S>` | стиль как run-length | `Decoration` (индикаторы: волнистая линия ошибки, подсветка вхождений) и `ContractionState` строятся на нём |
| `PerLine` (`LineMarkers/Levels/State/Annotation/Tabstops`) | данные «на строку» | вставка/удаление строк синхронно сдвигает массивы (`InsertLine/RemoveLine`) |
| `ContractionState` | видимость/свёртка/высота строк | doc-line ↔ display-line за O(log n); нужна для folding и wrap |
| `Document` | фасад модели | слова/классы символов (`CharClassify`), поиск (`RESearch`, `RegexSearchBase`), `DocWatcher` — уведомления `DocModification` (flags: вставка, удаление, смена стиля, смена маркера, before/after) |
| `Selection` | множественные выделения | вектор `SelectionRange`, виртуальное пространство, rectangular |
| `Decoration` | индикаторы | отдельный `RunStyles` на каждый индикатор |
| `UniConversion`, `DBCS`, `CharacterCategoryMap`, `CaseConvert/CaseFolder` | юникод | позиции — **байты UTF-8**; для UTF-16/UTF-32 есть индекс строк (`LineCharacterIndexType`) |

### 1.2 Вид (View) — переносим идеи, рисование свое
- `ViewStyle` + `Style`: таблица стилей (шрифт, цвета, регистр, hotspot), поля (`MarginStyle`: номера, маркеры, fold), цвета выделения/каретки/подсвеченной строки, EOL-аннотации.
- `EditView`: рисует строки; `LineLayout` (раскладка строки: позиции символов в пикселях, wrap-разрывы, BiDi) кэшируется в `LineLayoutCache`; `BreakFinder` делит строку на сегменты одинакового стиля/вида; `SpecialRepresentations` (отображение управляющих символов, `<LF>`).
- `MarginView`: рисует поля слева (номера, маркеры `LineMarker`, fold-линии, change history).
- `Editor` (9187 строк!) — «бог-класс»: курсор/выделение, скролл, политика видимости каретки (`CaretPolicies`), перенос строк (`WrapPending`, фоновый), ввод, drag&drop, clipboard-логика, **idle-работа** (`WorkNeeded`: стилизация, перенос порциями, чтобы не блокировать UI), все `WndProc`-сообщения.
- `ScintillaBase`: надстройка — автодополнение (`AutoComplete`, `ListBox`), `CallTip`, контекстное меню, подключение лексера (`LexState`).
- Платформа: события мыши/клавиш/таймеров/IME/clipboard/DnD, `Surface` (шрифты, измерение, градиенты, RGBA-маркеры).

### 1.3 Публичный API
- ~600 сообщений `SCI_*` (Scintilla.iface — декларативное описание, из него генерируются обёртки). Всё через один `SendMessage(msg, wParam, lParam)`.
- `IDocument` (чтение текста/стилей) + `ILexer5` (`Lex/Fold` вызываются на диапазоне с начальным стилем и **состоянием строки** `GetLineState`) — это и есть контракт «лексер раскрашивает инкрементально»: Scintilla находит начало строки перед изменением, берёт `LineState` предыдущей строки и перераскрашивает вперёд до тех пор, пока стили не перестанут меняться.
- `ILoader` — асинхронная загрузка больших файлов без блокировок.

## 2. Требования, которые решает Scintilla и которые нам важны

1. Большие файлы: gap-буфер, `Partitioning`, ленивый layout только видимых строк, `ILoader`.
2. Инкрементальная подсветка без пересчёта всего файла; **фоновая** (idle), в порядке «сначала видимое».
3. Корректные позиции при правках: все «на строку» данные (маркеры, fold, annotations, line state) смещаются вместе с текстом.
4. Undo/redo со слиянием, save point, «изменено с последнего сохранения».
5. Юникод: UTF-8, графемы/комбинируемые, BiDi, wide/emoji, переводы строк CR/LF/CRLF/NEL/LS/PS.
6. Множественные каретки/выделения, прямоугольное выделение, виртуальное пространство.
7. Fold (по `LineLevels`), wrap (`ContractionState` + layout), табы/отступы, guides, edge.
8. Индикаторы (ошибки/предупреждения/выделение слов), аннотации и «призрачный» текст (inline hints/EOL-аннотации).
9. Автодополнение/подсказки/calltip как отдельный popup-слой.
10. Производительность рисования: кэш раскладки, двойная буферизация, частичная инвалидация.
11. IME (в т.ч. tentative-вставка), доступность (a11y), DnD, clipboard (в т.ч. rectangular).
12. Мера «безопасности»: `Failure` при нехватке памяти, read-only, `SC_MOD_*` до/после изменения.

## 3. Где у нас что живёт (предложение)

Гибрид «UNO там, где нужна граница; C++ внутри, где важна скорость»:

```
codeapi (IDL)                   codeservices (UNO, без GUI)            GUI (exe/sidebar)
────────────────────            ───────────────────────────            ──────────────────────────
XTextBuffer*  (док. модель) ──▶ TextBuffer: GapBuffer, LineIndex,   ──▶ EditorControl (weld::CustomWidget)
XDocument.. уже есть            Undo, ChangeHistory, PerLine            ├ ViewStyle / StyleTable
XSemanticTokens (новый)    ◀──  сервис анализа (языки)                  ├ LineLayoutCache
XDecorations / XDiagnostics     Decorations, Folds, Diagnostics         ├ MarginRenderer
XCompletionProvider (новый) ◀── сервис анализа                          ├ Selection/Caret, Wrap, Scroll
                                                                        └ Completion/CallTip popups
```

- **Модель (UNO-компонент)**: `TextBuffer` (текст + ChangeHistory + Undo) — переиспользуем в тестах без GUI, она же питает сервис анализа. Это ядро «перенести почти один-в-один»: `GapBuffer`, `LineIndex`(partitioning), `RunStyles`, `PerLine`, `UndoHistory`, `Selection`. Реализуем на C++ внутри библиотеки (можно собственную, можно сначала на `std::` + gap vector), наружу — IDL-интерфейсы: текст/строки/правки/события (`DocumentEvent` уже есть).
- **Подсветка**: вместо `ILexer` — **потребляемые** данные: сервис анализа отдаёт `sequence<SemanticSpan>{Range, TokenKind, Modifiers}` и `Diagnostic{Range, Severity, Message}`, `FoldRange`. Инкрементальность: клиент помечает «грязные» диапазоны при `DocumentEvent`, сервис отвечает асинхронно по версии документа (`XBackgroundJob`), UI до прихода ответа рисует старые спаны, **смещённые** правками (как `RunStyles`). Лексер-состояния строки (`LineState`) нам не нужны — это забота сервиса.
- **Тема**: `TokenKind → Style` (`StyleTable`) — на стороне UI (цвета/шрифт), чтобы сервис анализа оставался независимым от UI; поддержка тёмной темы бесплатно.
- **GUI**: `weld::CustomWidget` с ручной отрисовкой на `OutputDevice` (кэш `LineLayout`, margins, caret, selection). `Editor` не копируем как гигант: делим на `CaretController`, `ScrollController`, `WrapEngine`, `InputHandler`, `ClipboardHandler`.
- **Popups** (completion, calltip): отдельные виджеты, данные от `XCompletionProvider`.

## 4. Что НЕ берём
Платформенные слои, весь `SCI_*`-протокол (у нас типизированные UNO-интерфейсы), лексеры, печать (`FormatRange`), `SC_PHASES`/BiDi-режимы на первой волне, DBCS-кодовые страницы (только UTF-8 внутри), regex-движок `RESearch` (берём `i18nutil`/ICU из LO).

## 5. Предлагаемые волны
1. **Модель** (без GUI, с тестами): GapBuffer, LineIndex, TextBuffer, UndoHistory, ChangeHistory, события; `XDocument`-интеграция, cppunit.
2. **Декорации и per-line**: RunStyles, Decorations, PerLine (маркеры, fold levels), смещение при правках; IDL `XSemanticTokens`/`XDiagnostics` и заглушка-сервис.
3. **Редактор-виджет read-only**: ViewStyle, StyleTable, LineLayoutCache, margins (номера, маркеры), скролл, подсветка из сервиса.
4. **Редактирование**: caret/selection/мульти-каретки, ввод, undo, clipboard, IME, отступы.
5. **Wrap, fold, индикаторы, аннотации**.
6. **Completion/CallTip**, a11y, DnD.

## 6. Открытые вопросы
- Свой `CustomWidget` против расширения `weld::TextView` (последний не даёт ни margins, ни индикаторов — упрёмся сразу).
- Позиции в API: UTF-16 (как UNO/LO) или байты UTF-8 (как Scintilla)? Рекомендую UTF-16 снаружи, внутренний буфер — UTF-8 + индекс по запросу.
- Есть ли уже контракт сервиса анализа (в deepcode — `lsp`/`navigation`)? Нужна ли в нём семантическая раскраска поверх LSP-модели (semanticTokens)?

## 7. Что показал deepcode (референс, не зависимость)

**Как у них:** редактор — это UNO-объекты без пикселей: `XEditorManager` → `XEditor` (+`XCursor`, `XSelection`, `XTextMark`), документ `XTextDocument` (`getText/getLine/applyEdit(TextEditEvent)`), а виджет — тонкий `XEditorWidget` (open/reveal/selectRange/focus по строковому `editorId`). Подсветка — плагины-провайдеры, регистрируемые в контроллере: `XSyntaxHighlighter` (на **строку**/диапазон строк → `Token{Offset,Length,Kind(short),Subkind}`), `XSemanticTokenProvider` (LSP-стиль `SemanticToken{line,startChar,length,type,modifiers}`), `XFoldingProvider`, `XDiagnosticProvider`, `XDocumentHighlightProvider`, `XInlayHintProvider`, `XSignatureHelpProvider`, `XCompletionProvider`, `XIndenter`. Все провайдеры имеют `getIdentifier()` + `getSupportedMimeTypes()`, контроллер выбирает по MIME. Язык — `XLanguageSupport` (parser + codeModel + symbolIndex), `BackgroundParser` слушает события документа и парсит автоматически.

**Что не сработало (учесть):**
- Подсветка — `highlightLine(uri, line)` без состояния строки: многострочные комментарии/строки невозможны (в `CppHighlighter` «блочные комментарии только однострочные»). У Scintilla для этого `LineState`.
- Нет понятия версии документа и инкрементальности: провайдеры вызываются синхронно «дай всё», нет отмены/устаревания результатов.
- `registerHighlighter` в `EditorManager` — пустая заглушка; 14 копипастных лексеров по одному .so на язык.
- Редактор-объекты (`XCursor`/`XSelection`) отделены от реального виджета: два источника истины для каретки; одно выделение, мульти-каретки невозможны.
- Нет модели маркеров на строку/полей: `XTextMark` с `PropertyValue[]` и `short Kind`, адресация по строковым id; нет fold-состояния, wrap, undo-интерфейса, ChangeHistory.
- `Kind` — магические short вместо enum/констант-набора, позиции `Offset` без указания кодировки.

**Что берём как есть по духу:** разбивка на провайдеры с mime-диспетчеризацией и регистрацией в контроллере; `Diagnostic`/`FoldingRange`/`SemanticToken`/`InlayHint` как отдельные небольшие структуры; `TextEditEvent{Range,NewText}` как единая форма правки; `BackgroundParser` на событиях документа.

## 8. Корректировка плана с учётом обоих источников

- Сервис анализа = набор **провайдеров с версией документа**: `provide…(url, int version, Range)` + `XBackgroundJob` с отменой; результат тегируется версией, UI отбрасывает устаревший и до прихода нового рисует старый, сдвинутый правками (RunStyles-логика Scintilla).
- Для лексеров, не LSP-ориентированных (быстрый встроенный разбор на ходу), нужен тот же контракт, что у Scintilla: `startState` на входе диапазона и `endState` на выходе (`LineState`) — иначе многострочные конструкции невозможны. Для semantic tokens состояние внутри сервиса.
- Единый источник истины для каретки/выделения — **в модели редактора** (`XEditorView`: мульти-выделения как `sequence<Range>`), а виджет только отображает и шлёт ввод. Это то, что в deepcode разведено на два слоя.
- `TokenKind` — UNO `enum` (+ `Modifiers` как constants-битмаска), не short.
- Один компонент `codeservices`, а не .so на язык (как уже решено в плане Code).
