const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="utf-8"/>
<title>Controle Restaurantes e Bares</title>
</head>
<body style="background-color:#708090">  
<div style="margin-left: 30%;margin-right:35% ;height:30em;background-color: #FFF;padding-top: 20px;border-top: 20px solid #4682b4;border-radius:5px; " >
<div style="margin:20%;text-align: center;border:0px ;  " >
<form action="/get">
<h2>Rede:</h2>
<input type='text' name='rede' style="width:98%;height:1.5em;background-color: ;font-size: 20px;border-radius: 5px;">
<h2>Senha:</h2>
<input type='text' name='senha_wifi' style="width:98%;height:1.5em;background-color: ;font-size: 20px;border-radius: 5px;"><br><br>
<input type='submit' value = 'salvar' style="width: 98%;height: 2.5em;background-color: #4682b4;color: white;font-size: 20px;border-radius: 20px;"> 

</form>
</div>
</div>


</body>
</html>
)=====";
