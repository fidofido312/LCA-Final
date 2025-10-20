TRABAJO PRÁCTICO FINAL 

1. Un proyecto de obra (obra original completa o una etapa de la misma desarrollada a los fines del curso).
3. Un Instrumento (una creación de luthería digital, es decir un software que ofrezca apertura y versatilidad para su uso en diferentes obras, en vez de una creación ad hoc en función de una pieza en particular con una poética cerrada).


Sistema visual 
Uso de microcontrolador esp-32
Objeto de estudio central Biomateriales y su Conductividad. 
Condicionales e iteraciones

Forma de vida infotecnológica.
Especie de cúmulo hecho de biomaterial. El cual contiene en su estructura hilos de cobre. Los mismos sobresalen de la materia y son para interactuar con les espectadores. 
Supongamos que del centro circular de biomaterial salen hacia afuera 5 hilos de cobre. Los espectadores pueden tomar con sus manos cada hilo. El sistema de la obra sensa la presencia de cada participante que toma uno de los tentáculos. Cuando los 5 tentáculos son tomados por distintos participantes el centro de la obra se ilumina en su totalidad. Ahora cuando un participante suelta un tentáculo la región central que corresponde a ese tentáculo se apaga. La obra se va apagando a medida que los tentáculos quedan sin ser sujetos. Existe un estado de iluminación total (cuando todos los tentáculos son tomados por distintos participantes), un estado de apagón total (cuando todos los tentáculos son soltados por participantes) y estados intermedios en donde cada región se enciende según el tentáculo que está siendo tomado por el participante. 

Expansión: memoria de la obra - El sistema almacena las distintas variaciones de interacción que fue teniendo con el público y las guarda como patrones de iluminación. Cuando la obra queda en estado de apagón por determinada cantidad de minutos la obra comienza a recordar y a pasar cada una de las iteraciones desde x cantidad de horas. 

La obra aborda dos temas de estudio abordados en el temario. Por un lado los condicionales y por el otro las iteraciones.


ESP32_Touch_LED

📌 ¿Qué hace el programa?
Lee un sensor táctil (touchRead).
Si el valor detectado es menor al umbral (30), enciende el LED.
Si no, lo apaga.
Imprime el valor leído en el Monitor Serial.
