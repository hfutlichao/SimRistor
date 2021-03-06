//===- FBLCController.cpp --------------------------------------------------*- C++ -*-===//
//
//  Copyright (C) 2017  Mario Barbareschi (mario.barbareschi@unina.it)
// 
//  This file is part of simristor.
//
//  simristor is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  simristor is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with Clang-Chimera. If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
/// \file FBLCController.cpp
/// \author Mario Barbareschi
/// \brief This file implements the class of a the controller of a FBLC
//===----------------------------------------------------------------------===//

#include <assert.h>     /* assert */
#include <algorithm>
#include <utility> /*pair*/
#include "FBLCController.hpp"
#include "utils.h"

std::string codeColor(simristor::XBarBlock blk){
    std::string ret;
    switch(blk){
        case simristor::XBarBlock::IN:
            ret = "\033[0;43;30m";
            break;
        case simristor::XBarBlock::MINTERM:
            ret = "\033[0;44;30m";
            break;
        case simristor::XBarBlock::AND:
            ret = "\033[0;45;30m";
            break;
        case simristor::XBarBlock::OL:
            ret = "\033[0;46;30m";
            break;
        case simristor::XBarBlock::NONE:
        default:
            ret = "\033[0;47;30m";
            break;
    }
    return ret;
}


simristor::FBLCController::FBLCController(XBar* crossbar, int inputRow, int inputs, int minterms, int outputs){
    this->crossbar = crossbar;
    this->inputRow = inputRow;
        
    int i, j = 0;
    //The following block tries to populate two structures
    //The first one is inputColumns that maps input memristors to a vector that includes minterm memristors which belongs to the same column
    //The second one is mintermRows that maps i-th crossbar rows to a vector that includes minterm memristors which belongs to the same i-th row
    for(j = 0; j < crossbar->getColumns(); j++){
            if(crossbar->getMemristor(inputRow,j)){
                inputMemristor.insert(std::make_pair(j,crossbar->getMemristor(inputRow,j)));
                
                DEBUG_PRINT("Found an input memristor at <"+std::to_string(inputRow)+", "+std::to_string(j)+">" << std::endl);
                
                for(i = 0; i < crossbar->getRows(); i++){
                    if(i != inputRow && crossbar->getMemristor(i, j)){
                        if(inputColumns.end() == inputColumns.find(crossbar->getMemristor(inputRow,j))){
                            inputColumns.insert(std::make_pair(
                                crossbar->getMemristor(inputRow,j), std::vector<std::shared_ptr<Memristor>>()
                            ));
                        }
                        inputColumns.find(crossbar->getMemristor(inputRow,j))->second.push_back(crossbar->getMemristor(i,j));
                        
                        if(mintermRows.end() == mintermRows.find(i)){
                            mintermRows.insert(std::make_pair(i, std::vector<std::shared_ptr<Memristor>>() ));
                        }
                        mintermRows.find(i)->second.push_back(crossbar->getMemristor(i,j));
                        
                        DEBUG_PRINT("Found a minterm memristor at <"+std::to_string(i)+", "+std::to_string(j)+">" << std::endl);
                    }
                }
            }
    }
    
    assert(inputs == 0 || inputs != 0 && inputColumns.size() == 2*inputs && "Number of input memristor does not match the literals!");
    
    this->inputs = inputColumns.size();

    assert(minterms == 0 || minterms != 0 && mintermRows.size() == minterms && "Number of minterms does not match xbar structure!");
    
    this->minterms = mintermRows.size();
    
    //The following block prepares two structures
    //The first one is mintermOutputRows that maps i-th crossbar rows to a vector that includes minterm memristors output which belongs to the same i-th row
    //The second one is outputColumns that maps j-th crossbar column to a vector that includes memristors output which belongs to the same j-th column
    for(j = 0; j < crossbar->getColumns(); j++){
        if(!crossbar->getMemristor(inputRow,j)){
            for(i = 0; i < crossbar->getRows(); i++){
                if(i != inputRow && mintermRows.end() != mintermRows.find(i) && crossbar->getMemristor(i,j)){
                    if(mintermOutputRows.end() == mintermOutputRows.find(i)){
                        mintermOutputRows.insert(std::make_pair(i, std::vector<std::shared_ptr<Memristor>>()));
                    }
                    mintermOutputRows.find(i)->second.push_back(crossbar->getMemristor(i,j));
                    DEBUG_PRINT("Found a minterm output memristor at <"+std::to_string(i)+", "+std::to_string(j)+">" << std::endl);
                    
                    if(outputColumns.end() == outputColumns.find(j)){
                        outputColumns.insert(std::make_pair(j, std::vector<std::shared_ptr<Memristor>>()));
                    }
                    outputColumns.find(j)->second.push_back(crossbar->getMemristor(i,j));
                }
            }
        }
    }
    
        
    
    //The following block prepares outputNegMemristor, which map the output column with the output inverted memristor
    for(i = 0; i < crossbar->getRows(); i++){
        //Exclusing all minterm and input rows
        if(i != inputRow && mintermOutputRows.end() == mintermOutputRows.find(i)){
            for(j = 0; j < crossbar->getColumns(); j++){
                //Excluding columns which are under input memristors and minterms
                if(!crossbar->getMemristor(inputRow,j) && crossbar->getMemristor(i,j)){
                    if(outputColumns.end() != outputColumns.find(j)){
                        //This is a inverted memristor output
                        outputNegMemristorColumn.insert(std::make_pair(j,crossbar->getMemristor(i,j)));
                        outputNegMemristorRow.insert(std::make_pair(i,crossbar->getMemristor(i,j)));
                    } else {
                        //This is a directed memristor output
                        outputMemristorRow.insert(std::make_pair(i,crossbar->getMemristor(i,j)));
                    }
                }
            }
        }
    }
    

    assert(outputs == 0 || outputs != 0 && outputColumns.size() == outputs && "Number of output does not match xbar structure!");
    assert(outputs == 0 || outputs != 0 && outputNegMemristorRow.size() == outputs && "Number of output does not match xbar structure!");
    this->outputs = outputNegMemristorRow.size();

    xbarStructure.reserve(crossbar->getRows()*crossbar->getColumns());
    std::fill(xbarStructure.begin(), xbarStructure.end(), simristor::XBarBlock::NONE);
    
    for(auto inMem : inputMemristor){
        xbarStructure[inputRow*crossbar->getColumns() + inMem.first] = simristor::XBarBlock::IN;
        for(auto row : mintermRows){
            xbarStructure[row.first*crossbar->getColumns() + inMem.first] = simristor::XBarBlock::MINTERM;
        }
    }
    
    for(auto row : mintermOutputRows)
        for(auto col : outputColumns)
            xbarStructure[row.first*crossbar->getColumns() + col.first] = simristor::XBarBlock::AND;

    for(auto row : outputMemristorRow){
        int i;
        for(i = 0; i < crossbar->getColumns(); i++)
            if(crossbar->getMemristor(row.first, i))
                xbarStructure[row.first*crossbar->getColumns() + i] = simristor::XBarBlock::OL;
    }
}


simristor::FBLCController* simristor::FBLCController::ina(){
    DEBUG(clock_t begin = clock());
    int i, j;
    for(i = 0; i < crossbar->getRows(); i++){
        for(j = 0; j < crossbar->getColumns(); j++){
            auto memptr = crossbar->getMemristor(i,j);
            if(memptr)
                memptr->set();
        }
    }
    DEBUG(double elapsed_secs = double(clock() - begin) / (CLOCKS_PER_SEC/1000));
    DEBUG_PRINT("INA successfully completed in "<< elapsed_secs << " ms" <<std::endl);

    return this;
}

simristor::FBLCController* simristor::FBLCController::ri(bool* inputValues){
    DEBUG(clock_t begin = clock());
    
    for(auto& inputMem : inputMemristor)
        *inputValues++ ? inputMem.second->reset() : inputMem.second->set();
    
    DEBUG(double elapsed_secs = double(clock() - begin) / (CLOCKS_PER_SEC/1000));
    DEBUG_PRINT("RI successfully completed in "<< elapsed_secs << " ms" <<std::endl);        
    return this;
}

simristor::FBLCController* simristor::FBLCController::cfm(){
    DEBUG(clock_t begin = clock());
    for(auto column : inputColumns)
        if(!column.first->isHigh())
            for(auto term : column.second)
                *term = *column.first;
    
    DEBUG(double elapsed_secs = double(clock() - begin) / (CLOCKS_PER_SEC/1000));
    DEBUG_PRINT("CFM successfully completed in "<< elapsed_secs << " ms" <<std::endl);
    return this;
}

simristor::FBLCController* simristor::FBLCController::evm(){
    DEBUG(clock_t begin = clock());
    //For each row of minterm output memristors
    for(auto mintermValue : mintermOutputRows){
        //Retrieve the corresponsing minterm memristors (i.e. ones belonging to the same row)
        auto mintermLiteralsVector = mintermRows.find(mintermValue.first)->second;
        //Search for minterm memristors which are set to logic-0 value
        auto it = std::find_if(mintermLiteralsVector.begin(), mintermLiteralsVector.end(), [&](std::shared_ptr<simristor::Memristor> const& memristor) {
            return *memristor == simristor::Memristor(simristor::LOW);
        });
        //If none is found, we need to switch-off all minterm output memristors 
        if (it == mintermLiteralsVector.end()) { /* do what you want with the value found */ 
            for(auto value : mintermValue.second)
                value->reset();
        }
    }
    DEBUG(double elapsed_secs = double(clock() - begin) / (CLOCKS_PER_SEC/1000));
    DEBUG_PRINT("EVM successfully completed in "<< elapsed_secs << " ms" <<std::endl);
    return this;
}

simristor::FBLCController* simristor::FBLCController::evr(){
    DEBUG(clock_t begin = clock());
    for(auto invertedOutput : outputNegMemristorColumn){
        //Retrieve the corresponding memristor of the inverted output
        auto mintermOutputValue = outputColumns.find(invertedOutput.first)->second;
        //Search for minterm memristors which are set to logic-0 value
        auto it = std::find_if(mintermOutputValue.begin(), mintermOutputValue.end(), [&](std::shared_ptr<simristor::Memristor> const& memristor) {
            return *memristor == simristor::Memristor(simristor::LOW); // assumes MyType has operator==
        });
        //If at least one is found, we need to switch-off all minterm output memristors 
        if (it != mintermOutputValue.end()) { /* do what you want with the value found */ 
                invertedOutput.second->reset();
        }
    }
    DEBUG(double elapsed_secs = double(clock() - begin) / (CLOCKS_PER_SEC/1000));
    DEBUG_PRINT("EVR successfully completed in "<< elapsed_secs << " ms" <<std::endl);
    return this;
}

simristor::FBLCController* simristor::FBLCController::inr(){
    DEBUG(clock_t begin = clock());
    
    //Retrieve the directed output memristor
    for(auto outMem : outputMemristorRow)
        if(outputNegMemristorRow.find(outMem.first)->second->isHigh())
            outMem.second->reset();
    
    DEBUG(double elapsed_secs = double(clock() - begin) / (CLOCKS_PER_SEC/1000));
    DEBUG_PRINT("INR successfully completed in "<< elapsed_secs << " ms" <<std::endl);
    return this;
}

simristor::FBLCController* simristor::FBLCController::so(bool* outputValues){
    DEBUG(clock_t begin = clock());

    for(auto& outMem : outputMemristorRow)
        *outputValues++ = outMem.second->isHigh();
    
    DEBUG(double elapsed_secs = double(clock() - begin) / (CLOCKS_PER_SEC/1000));
    DEBUG_PRINT("SO successfully completed in "<< elapsed_secs << " ms" <<std::endl);
    return this;
}

const simristor::XBar* simristor::FBLCController::getXBar() const{
    return crossbar;
}


void simristor::FBLCController::print(std::ostream& outstream){
   outstream << "\033[1m" << crossbar->getName() << "\033[0m: " << crossbar->getRows() << "x" << crossbar->getColumns() << std::endl;
    int i, j;
    for(i = 0; i < crossbar->getRows(); i++){
        for(j = 0; j < crossbar->getColumns(); j++)
            outstream << codeColor(xbarStructure[i*crossbar->getColumns()+j] ) << "    |";
        outstream << "\033[0;47;30m" << "  " << "\033[0m" << std::endl <<  "\033[0m" ;
        for(j = 0; j < crossbar->getColumns(); j++){
            if(crossbar->getMemristor(i,j) != nullptr)
                if(crossbar->getMemristor(i,j)->isHigh())
                    outstream << codeColor(xbarStructure[i*crossbar->getColumns()+j] ) << "___\033[0;31m/" << codeColor(xbarStructure[i*crossbar->getColumns()+j] ) <<"|";
                else
                    outstream << codeColor(xbarStructure[i*crossbar->getColumns()+j] )  << "___\033[0;32m/" << codeColor(xbarStructure[i*crossbar->getColumns()+j] ) <<"|";
            else
                outstream << codeColor(xbarStructure[i*crossbar->getColumns()+j] ) << "____|";
        }
        outstream << "\033[0;47;30m" << "__" << "\033[0m" << std::endl <<  "\033[0m" ;
    }
    for(j = 0; j < crossbar->getColumns(); j++)
            outstream << codeColor(simristor::XBarBlock::NONE) << "    |";
        outstream << "\033[0;47;30m" << "  " << "\033[0m" << std::endl <<  "\033[0m" ;
}

simristor::FBLCController::~FBLCController(){
    xbarStructure.clear();
    inputMemristor.clear();
    inputColumns.clear();
    mintermRows.clear();
    mintermOutputRows.clear();
    outputColumns.clear();
    outputNegMemristorColumn.clear();
    outputNegMemristorRow.clear();
    outputMemristorRow.clear();
}
