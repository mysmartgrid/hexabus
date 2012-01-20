transTable = new Array();
condTable = new Array();
transDT = false;
condDT = false;

function alterValue(dtype, divID) {
			if(dtype == 4) {
				document.getElementById(divID+'Simple').style.display = 'none';
				document.getElementById(divID+'Complex').style.display = '';
				if(divID == "cv") {
					condDT = true;
				} else {
					transDT = true;
				}
			} else {
				if(divID == "cv") {
					condDT = false;
				} else {
					transDT = false;
				}
				document.getElementById(divID+'Simple').style.display = '';
				document.getElementById(divID+'Complex').style.display = 'none';
			}
		}

		function generateTables() {
			var destForm = document.getElementById("sendingForm");
			var hiddenInput = document.getElementById("generatedTables");
			hiddenInput.value = decomposeTable(condTable);
			hiddenInput.value += decomposeTable(transTable);
		}
		
		function decomposeTable(table) {
			var tableString = "-";
			for(var i = 0;i < table.length;i++) {
				for(var j = 0;j < table[0].length;j++) {
					tableString += table[i][j];
					tableString += ".";
				}
			}
			tableString += ".";
			return tableString;
		}

		function addToDropDownMenu(i) {
			var dropDown = document.getElementById("condID");
			var option = document.createElement("option");
			option.text = i;
			option.value = i;
			dropDown.add(option, null);
		}
		function addTableLine(formName, tableName) {
			var form = document.getElementById(formName);
			var table = document.getElementById(tableName);
			var row = table.insertRow(table.rows.length);
			var tmpArray = new Array();
			
			if(tableName == "condTable") {
				var j = 0;
				row.insertCell(0).innerHTML = condTable.length;
				j++;
				for(var i = 0;i < form.length;i++, j++) {
					if(i == 3) {
						if(condDT) {
							i++;
							var op = Number(form.elements[i].value) + Number(form.elements[i+1].value);
							row.insertCell(j).innerHTML = op;
							tmpArray.push(op);
							i++;
						} else {
							row.insertCell(j).innerHTML = form.elements[i].value;
							tmpArray.push(form.elements[i].value);
							i += 2;
						}
					} else {
						row.insertCell(j).innerHTML = form.elements[i].value;
						tmpArray.push(form.elements[i].value);
					}
				}
				tmpArray[0] = (String(tmpArray[0]).replace(/:/g, ""));
				condTable.push(tmpArray);
				addToDropDownMenu(condTable.length - 1);
			}
			if(tableName == "transTable") {
				for(var i = 0, j = 0;i < form.length;i++, j++) {
					if(i == 4) {
						if(transDT) {
							var val = "";
							for(var k = 0;k < 7;k++) {
								val += form.elements[k+5].value;
								val += "*";
							}
							row.insertCell(j).innerHTML = val;
							tmpArray.push(val);
							i += 7;
						}
						else {
							row.insertCell(j).innerHTML = form.elements[i].value;
							tmpArray.push(form.elements[i].value);
							i += 7;
						}
					} else {
						row.insertCell(j).innerHTML = form.elements[i].value;
						tmpArray.push(form.elements[i].value);
					}
				}
				transTable.push(tmpArray);
			}
	}
