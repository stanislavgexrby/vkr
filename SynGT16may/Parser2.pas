unit Parser2;
{
Grammatika razbora
RE_FromString=E'.'
E=T[';'T]*
T=F[','F]*
F=U[*U]*
U=$Semantic; Term; NonTerm;'(' E ')'; '[' E ']'
Term=''',Char*,'''
}
interface
uses RegularExpression,CharProducer, Parser;

function RE_FromCharProducer2(ACharProducer:TCharProducer; Grammar:TGrammar): TRE_Tree;

implementation
uses ErrorUnit,SysUtils;

function E(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;forward;

function U(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
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
        Result:=TRE_Or.make(CurGrammar,TRE_Terminal.makeFromID(CurGrammar,0),first);
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
      name:={UpperCase}(readIdentifier(ACharProducer));
      f:=CurGrammar.findNonterminalName(name);
      if f=-1 then f:=CurGrammar.addNonterminalName(name);
      result:=TRE_NonTerminal.makeFromID(CurGrammar, f)
    end;
  end;
end;

function F(ACharProducer:TCharProducer;curGrammar:TGrammar):TRE_Tree;
var
  first: TRE_Tree;
begin
  first:=U(ACharProducer,curGrammar);
  while ACharProducer.CurrentChar='*' do
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


function RE_FromCharProducer2(ACharProducer:TCharProducer; Grammar:TGrammar): TRE_Tree;
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
end.
