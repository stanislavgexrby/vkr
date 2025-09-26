unit TransParser;
{
Grammatika razbora
RE_FromString=E'.'
E=T[';'T]*
T=F[','F]*
F=U[#U]*
U=K['*';'+']*
K=$NewSemantic;Term; NonTerm; Semantic; Macro; '(' E ')'; '[' E ']'
}
interface
uses TransRE_Tree, TransGrammar,TransCharProducer;

{function RE_FromCharProducer(ACharProducer:TCharProducer; Grammar:TGrammar): TRE_Tree;}

procedure SkipSpaces(ACharProducer:TCharProducer);
procedure SkipToChar(ACharProducer:TCharProducer; ch:char);
function ReadIdentifier(ACharProducer:TCharProducer):string;
function ReadName(ACharProducer:TCharProducer; lastChar:char):string;
function isLetterOrDigit(ch:char):boolean;

implementation
uses TransRE_Or, TransRE_And, TransRE_Iteration, TransRE_Nonterminal, TransRE_Terminal, TransRE_Semantic,
     TransErrorUnit,SysUtils;

{function E(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;forward;}

procedure SkipNotMatter(ACharProducer:TCharProducer);
begin
  while not(ACharProducer.CurrentChar in [#0,'0'..'9',
       'A'..'Z','a'..'z','_','''','"','[',']','(',')',
       '{','}','*','+',',',';','#','.','&','@',':','$','/']) do
    ACharProducer.next;
end;

procedure SkipToChar(ACharProducer:TCharProducer; ch:char);
begin
  while (ACharProducer.CurrentChar <> ch)
    and (ACharProducer.next) do;
end;
procedure SkipSpaces(ACharProducer:TCharProducer);
begin
  SkipNotMatter(ACharProducer);
  while ACharProducer.CurrentChar in ['{', '/'] do
  begin
    if ACharProducer.CurrentChar='{' then
      skipToChar(ACharProducer,'}')
    else
      skipToChar(ACharProducer,#13);
    ACharProducer.next;
    SkipNotMatter(ACharProducer);
  end;
end;

function ReadName(ACharProducer:TCharProducer; lastChar:char):string;
var
  curChar:char;
  name:string;
begin
  curChar:=ACharProducer.CurrentChar;
  name:='';
  while (curChar<>lastChar) and ACharProducer.next do begin
    name:=name+curChar;
    curChar:=ACharProducer.CurrentChar;
  end;
  result:=name;
end;

function isLetterOrDigit(ch:char):boolean;
begin
  result:=((ch>='A') and (ch<='Z'))or
          ((ch>='a') and (ch<='z'))or
          ((ch>='0') and (ch<='9'))or (ch='_');
end;
function ReadIdentifier(ACharProducer:TCharProducer):string;
var
  curChar:char;
  name:string;
begin
  SkipSpaces(ACharProducer);
  curChar:=ACharProducer.CurrentChar;
  name:='';
  while isLetterOrDigit(curChar)and ACharProducer.next do begin
    name:=name+curChar;
    SkipSpaces(ACharProducer);
    curChar:=ACharProducer.CurrentChar;
  end;
  result:=name;
end;
{
function K(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  name:string;
  f:integer;
  first: TRE_Tree;
  curChar:char;
begin
  SkipSpaces(ACharProducer);
  curChar:=ACharProducer.CurrentChar;
  case curChar of
    '(': begin
      ACharProducer.next;
      first:=E(ACharProducer,curGrammar);
      SkipSpaces(ACharProducer);
      if ACharProducer.CurrentChar=')' then begin
        ACharProducer.next;
        skipSpaces(ACharProducer);
        Result:=first;
      end else begin
            ERROR(Format(CantFindChar,[')']));
            Abort;
      end;
    end;
    '[': begin
      ACharProducer.next;
      first:=E(ACharProducer,curGrammar);
      SkipSpaces(ACharProducer);
      if ACharProducer.CurrentChar=']' then begin
        ACharProducer.next;
        skipSpaces(ACharProducer);
        Result:=TRE_Or.Create(CurGrammar, CurGrammar.getTerminal(''), first);
      end else begin
            ERROR(Format(CantFindChar,[']']));
            Abort;
      end;
    end;
    '''','"': begin
      ACharProducer.next;//skip '''', '"'
      name:=readName(ACharProducer,curChar);
      if ACharProducer.next//skip '''', '"'
      then begin
        skipSpaces(ACharProducer);
        f:=CurGrammar.addTerminalName(name);
        result:=TRE_Terminal.makeFromID(CurGrammar,f);
      end else begin
            ERROR(Format(CantFindChar,[curChar]));
            Abort;
      end;
    end;
    '&','@': begin
      ACharProducer.next;
      skipSpaces(ACharProducer);
      result:=TRE_Terminal.makeFromID(CurGrammar,0);
    end;
    '$': begin
      ACharProducer.next;
      name:='$'+readIdentifier(ACharProducer);
      f:=CurGrammar.findSemanticName(name);
      if f<0 then f:=CurGrammar.addSemanticName(name);
      result:=TRE_Semantic.makeFromID(CurGrammar, f)
    end;
    else begin
      name:=readIdentifier(ACharProducer);
      f:=CurGrammar.find(cgNonTerminalList,name);
      if f>=0 then result:=TRE_NonTerminal.makeFromID(CurGrammar, f)
      else begin
        f:=CurGrammar.find(cgSemanticList,name);
        if f>=0 then result:=TRE_Semantic.makeFromID(CurGrammar, f)
        else begin
          f:=CurGrammar.FindMacroName(name);
          if f>=0 then
            result:=TRE_Macro.makeFromID(CurGrammar, f)
          else begin
            ERROR(Format(CantFindName,[name]));
            Abort;
          end;
        end;
      end;
    end;
  end;
end;
function U(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  first: TRE_Tree;
  curChar:char;
begin
  first:=K(ACharProducer,curGrammar);
  curChar:=ACharProducer.CurrentChar;
  while ACharProducer.CurrentChar in ['*','+'] do
  begin
    if curChar='+' then
      first:=TRE_Iteration.make(CurGrammar,first, TRE_Terminal.Create(CurGrammar))
    else
      first:=TRE_Iteration.make(CurGrammar,TRE_Terminal.Create(CurGrammar),first);
    ACharProducer.next;
    skipSpaces(ACharProducer);
  end;
  result:=first;
end;
function F(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  first: TRE_Tree;
begin
  first:=U(ACharProducer,curGrammar);
  while ACharProducer.CurrentChar='#' do
  begin
    ACharProducer.next;
    first:=TRE_Iteration.make(CurGrammar,first, U(ACharProducer,curGrammar));
  end;
  result:=first;
end;
function T(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  first: TRE_Tree;
begin
  first:=F(ACharProducer,curGrammar);
  while ACharProducer.CurrentChar=',' do
  begin
    ACharProducer.next;
    first:=TRE_And.make(CurGrammar,first,F(ACharProducer,curGrammar));
  end;
  result:=first;
end;
function E(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  first: TRE_Tree;
begin
  first:=T(ACharProducer,curGrammar);
  while ACharProducer.CurrentChar=';' do
  begin
    ACharProducer.next;
    first:=TRE_Or.make(CurGrammar,first,T(ACharProducer,curGrammar));
  end;
  result:=first;
end;


function RE_FromCharProducer(ACharProducer:TCharProducer; Grammar:TGrammar): TRE_Tree;
var
  first:TRE_Tree;
begin
  first:=E(ACharProducer,Grammar);
  if ACharProducer.CurrentChar='.' then begin
    result:=first;
  end else begin
    ERROR(Format(CantFindChar,['.']));
    Abort;
  end;
end;
}end.
