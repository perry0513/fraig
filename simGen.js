var fs = require('fs');
var l = process.argv[2];
var u = process.argv[3];

function createDofile(path){
	let output = ``;
	for(let i = l ; i <= u ; i++){
		let x = String(i);
		if(i < 10) 
			x = `0` + i;
		let ctx = fs.readFileSync(`./tests.fraig/sim${x}.aag`,'utf8');
		let data = /^aag\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)/g.exec(ctx);
		let miloa = [data[1],data[2],data[3],data[4],data[5]];
		output += `cirr tests.fraig/sim${x}.aag -r\n`;

		// TEST SIM
		output += `cirsim -f tests.fraig/pattern.${x} -o out${x}.log\n`;
		output += `cirp -fec\n`;
		let maxIndex = Number(miloa[0])+Number(miloa[3]);
		for(let j = 0; j <= maxIndex; j++)
			output += `cirg ${j}\n`;
//		for(let j = 0; j <= maxIndex; j++)
//			output += `cirg ${j} -fanout ${Math.floor(Math.random()*100)}\n`;
//		for(let j = 0; j <= maxIndex; j++)
//			output += `cirg ${j} -fanin ${Math.floor(Math.random()*100)}\n`;


		ctx = fs.readFileSync(`./dofraig`,'utf8');
		output += ctx;
	}
	output += `usage\n`;
	output += `q -f\n`;
	fs.writeFile(path[0], output, function(err) {
		if(err) {
			return console.log(err);
		}
	});
	fs.writeFile(path[1], output, function(err) {
		if(err) {
			return console.log(err);
		}
	});
}

createDofile(['./sim_mydo','./sim_refdo']);