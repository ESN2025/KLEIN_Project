# Projet de Co-Design - KLEIN

## Objectif 
L'énoncé du sujet indique l'objectif suivant :
>Display the gyroscope data of your DE10-Lite board with the following specs
>1. Get acceleration data each second
>2. Read acceleration along X,Y and Z axis
>3. Display their value on the 7seg display
>4. Swap between values using push button

## Hardware
![Organisation du projet](https://github.com/user-attachments/assets/ce5f0879-ae43-48bc-bb19-5fd63ccc2ba5)

### NIOS II / RAM / JTAG
Il s'agit ici de la configuration par défaut d'un NIOS, rien de particulier à signaler.

### TIMER
L'IP timer est utile ici pour générer des interruptions, celles-ci correspondent aux requêtes de lecture de l'accéléromètre. Le timer est réglé sur 0.1 seconde pour avoir une donnée fiable au cours du temps.

### OPENCORE
L'IP Opencore est une IP permettant d'interfacer des composant en I2C, elle nous permet ici de communiquer avec l'accéléromètre qui est interfaçable en SPI ou I2C.

### PIO OUT DATA
Ce PIO permet au software de piloter les afficheurs 7 segments. Il est d'une longueur de 24 bits, 14-0 représentant les entrées des décodeurs 7 segments "classique" et 23-15 représentant les entrées des décodeurs 7 segments "custom".

### PIO OUT LED
Ce PIO permet au software de piloter les 10 leds présentes sur la board. Le PIO est d'une longueur de 10 bits, avec une led par bit.

### PIO IN
Ce PIO permet au software de recevoir les interruptions générer par le bouton de changement d'axe.

### Bin 2 7 Segs
Ce composant est un décodeur 7 segments classique : il transforme un nombre en entrée en sa version 7 segments. Les nombres en entrée sont 0-F (en hex). Il prent également en entrée un bit suplémentaire pour afficher ou non le point.

### Bin 2 Custom 7 Segs
Ce composant est une version modifié du composant précédent, il supporte moins d'entrées mais permet d'afficher des caractères spécialement pour ce projet :
| Valeur d'entrée | Caractère affiché |
| :---: | :---: |
| 1 | = |
| 2 | X |
| 3 | Y |
| 4 | Z |
| 5 | - |
| autre | rien |

## Software

### Fonction I2C
L'IP opencore inclut des fonctions pour simplifier la communication I2C cependant la norme imposée par l'accéléromètre nous oblige à recréer encore d'autres fonctions pour ne pas saturer le code.

Norme de l'accéléromètre :
![Norme](https://github.com/user-attachments/assets/d129c41e-d409-4915-8a38-601530cad766)

Pour écrire dans un registre, il suffirat d'utiliser cette fonction :
```
  void AccWrite(int registerAdd, int value){
  	I2C_start(I2C_BASE,0x1D,0);
  	I2C_write(I2C_BASE,registerAdd,0);
  	I2C_write(I2C_BASE,value,1);
  	return;
  }
```

Pour lire dans un registre, il suffirat d'utiliser celle-ci :
```
  alt_u32 AccRead(int registerAdd){
  	I2C_start(I2C_BASE,0x1D,0);
  	I2C_write(I2C_BASE,registerAdd,0);
  	I2C_start(I2C_BASE,0x1D,1);
  	return I2C_read(I2C_BASE,1);
  }
```

Pour lire les registres de données (lecture consécutive de deux registres), il suffirat d'utiliser la fonction suivante :
```
  alt_u32 AccReadData(int registerAdd){
  	I2C_start(I2C_BASE,0x1D,0);
  	I2C_write(I2C_BASE,registerAdd,0);
  	I2C_start(I2C_BASE,0x1D,1);
  	alt_u32 data1 = I2C_read(I2C_BASE,0);
  	alt_u32 data2 = I2C_read(I2C_BASE,1);
  	return ((data2 << 2) + (data1 >> 6));
  }
```
*NB : Cette fonction de lecture de données fonctionne uniquement si les données sont justifié à gauche, c'est à dire sur le registre DATAx1)*

### Interruption

#### Timer
Le timer génère des interruptions qui vont déclencher l'éxécution de la fonction suivante : 
```
static void readSample(void * context, alt_u32 id){
	alt_u32 value = AccReadData(0x32 + sampleOffset);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_OUT_DATA_BASE,convertDataToDisplayFormat(value));
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_OUT_LED_BASE,ledId);
	//clear
	IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE,0);
}
```
La fonction s'éxécute en 3 temps :
1. Lecture de la donnée dans les registres de l'accéléromètre
2. Envoi sur les décodeurs 7 segments la valeur lu à l'étape 1 et envoie sur les leds de la commandes d'allumage.
3. Reset de l'interruption pour qu'elle ne soit pas redéclencher instantanément.

#### Bouton
Le bouton génère des interruptions qui déclenche cette fonction :
```
static void changeSampleOffset(void * context, alt_u32 id){
	if(sampleOffset==4){
		sampleOffset=0;
	}else{
		sampleOffset+=2;
	}
	//clear
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PIO_IN_BASE,0x01);
}
```
Cette fonction permet simplement de changer l'offset entre 0, 2 et 4. Cet offset est utilisé dans l'interruption pour choisir l'axe lu, en effet, les registres des différents axes se suivant, un simple décalage permet de choisir l'axe lu tel que :
| Offset | Axe lu |
| :---: | :---: |
| 0 | X |
| 2 | Y |
| 4 | Z |

On termine la fonction comme toutes les interruptions, c'est à dire avec un reset de l'interruption

### Initialisation
Pour initialiser le projet, certaines manipulation sont requisent :  
`I2C_init(I2C_BASE,ALT_CPU_FREQ,100000);` permet d'initialiser la communication I2C.  
`calibrateAcc();` est une fonction initialisant l'accéléromètre tout en le calibrant.  
```
  //TIMER IRQ
	alt_irq_register(TIMER_IRQ, NULL, (void*)readSample);
	//BUTTON IRQ
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PIO_IN_BASE, 0x01);
	alt_irq_register(PIO_IN_IRQ, NULL, (void*)changeSampleOffset);
```
On termine l'initialisation en enregistrant les interruptions.

## Conclusion
Ce projet de co-design a permit de réaliser un project concret sur FPGA en utilisant les connaissances du cours ainsi que les précédents TP. 
L'utilisation de l'accéléromètre met en lumière l'utilisation, premièrement d'IP pré-éxistante pour simplifier et accélérer la réalisation du projet et deuxièmement l'utilité de la partie software dans ce projet.
Une des pistes d'améliorations de ce projet est de modifier les décodeurs 7 segments et custom 7 segments pour premièrement ne faire qu'un seul composant multi-fonctionnel et ensuite de réaliser un IP de ce nouveau composant pour l'intégrer directement dans QSYS et donc de simplifier le projet.
