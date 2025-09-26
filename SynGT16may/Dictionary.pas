unit Dictionary;

interface
uses Classes;
function FindDictionaryName(const name:string):integer;

implementation
uses SysUtils,RegularExpression;
var DictionaryNames:TStringList;

function FindDictionaryName(const name:string):integer;
var
  f:integer;
  UpName:string;
begin
  UpName:={AnsiUpperCase}(name);
  f:=DictionaryNames.indexOf(UpName);
  if f<4 then result:=f
  else result:=cgSemanticList;
end;
begin
  DictionaryNames:=TStringList.Create;
  DictionaryNames.add('TERMINALS');
  DictionaryNames.add('FORWARDPASSSEMANTICS');
  DictionaryNames.add('NONTERMINALS');
  DictionaryNames.add('AUXILIARYNOTIONS');
  DictionaryNames.add('BACKWARDPASSSEMANTICS');
  DictionaryNames.add('FORWARDPASSRESOLVERS');
  DictionaryNames.add('BACKWARDPASSRESOLVERS');
end.
