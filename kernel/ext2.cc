#include "ext2.h"
#include "libk.h"
#include "utils.h"
#include "path.h"
;

Ext2* fs1 = nullptr;
Ext2::Ext2(StrongPtr<Ide> ide_): ide_(ide_), root() {
    using namespace impl::ext2;
    fs1 = this;

    ide_->read(1024,sb);
    iNodeSize = sb.inode_size;  
    iNodesPerGroup = sb.inodes_per_group;
    numberOfNodes = sb.inodes_count;
    numberOfBlocks = sb.blocks_count;

    //Debug::printf("inodes_count %d\n",sb.inodes_count);
    //Debug::printf("blocks_count %d\n",sb.blocks_count);
    //Debug::printf("blocks_per_group %d\n",sb.blocks_per_group);
    //Debug::printf("inode_size %d\n",sb.inode_size);
    //Debug::printf("block_group_nr %d\n",sb.block_group_nr);
    //Debug::printf("first_inode %d\n",sb.first_inode);

    blockSize = uint32_t(1) << (sb.log_block_size + 10);

    buffer_cache = StrongPtr<BufferCache>::make(ide_);
    
    nGroups = (sb.blocks_count + sb.blocks_per_group - 1) / sb.blocks_per_group;
    //Debug::printf("nGroups = %d\n",nGroups);
    ASSERT(nGroups * sb.blocks_per_group >= sb.blocks_count);

    auto superBlockNumber = 1024 / blockSize;
    //Debug::printf("super block number %d\n",superBlockNumber);

    auto groupTableNumber = superBlockNumber + 1;
    //Debug::printf("group table number %d\n",groupTableNumber);

    auto groupTableSize = sizeof(BlockGroup) * nGroups;
    //Debug::printf("group table size %d\n",groupTableSize);

    groupTable = new BlockGroup[nGroups];
    auto cnt = buffer_cache->read_all(groupTableNumber * blockSize, groupTableSize, (char*) groupTable);
    ASSERT(cnt == groupTableSize);

    iNodeTables = new uint32_t[nGroups];
    for (uint32_t i=0; i < nGroups; i++) {
        auto g = &groupTable[i];
    

        iNodeTables[i] = g->inode_table;

        //Debug::printf("group #%d\n",i);
        //Debug::printf("    block_bitmap %d\n",g->block_bitmap);
        //Debug::printf("    inode_bitmap %d\n",g->inode_bitmap);
        //Debug::printf("    inode_table %d\n",g->inode_table);

    }

    //Debug::printf("========\n");
    //Debug::printf("iNodeSize %d\n",iNodeSize);
    //Debug::printf("nGroups %d\n",nGroups);
    //Debug::printf("iNodesPerGroup %d\n",iNodesPerGroup);
    //Debug::printf("numberOfNodes %d\n",numberOfNodes);
    //Debug::printf("numberOfBlocks %d\n",numberOfBlocks);
    //Debug::printf("blockSize %d\n",blockSize);
    //for (unsigned i = 0; i < nGroups; i++) {
    //    Debug::printf("iNodeTable[%d] %d\n",i,iNodeTables[i]);
    //}

    //root = new Node(ide,2,blockSize);

    root = get_node(2);

    //root->show("root");

    //root->entries([](uint32_t inode, char* name) {
    //    Debug::printf("%d %s\n",inode,name);
    //});



    //println(sb.uuid);
    //println(sb.volume_name);
}

uint32_t Node::get_offset() {
        uint32_t idx = number - 1;
        uint32_t group = idx / fs->iNodesPerGroup;
        uint32_t idxGroup = idx % fs->iNodesPerGroup;
        uint32_t table = fs->iNodeTables[group];
        return table * block_size + idxGroup * fs->iNodeSize;
}
uint32_t Node::get_specific_offset(uint32_t num) {
        uint32_t idx = num - 1;
        uint32_t group = idx / fs->iNodesPerGroup;
        uint32_t idxGroup = idx % fs->iNodesPerGroup;
        uint32_t table = fs->iNodeTables[group];
        return table * block_size + idxGroup * fs->iNodeSize;
}
uint32_t Ext2::alloc_block() {
    char bitmap[blockSize];
    for (uint32_t bg = 0;  bg < nGroups; bg++) {
        // read the bitmap from disk
        uint32_t block_bitmap = groupTable[bg].block_bitmap;
        buffer_cache->read_all(block_bitmap * blockSize, blockSize, bitmap);

        uint32_t bpg = sb.blocks_per_group;
        for (uint32_t i = 0; i < bpg; i++) {
            uint32_t byte = i / 8;
            uint32_t bit = i % 8;
            if ((bitmap[byte] & (1 << bit)) == 0) {
                bitmap[byte] |= (1 << bit);
                // put the updated bitmap on disk
                buffer_cache->write_all(block_bitmap * blockSize, blockSize, bitmap);
                sb.free_blocks_count--;
                groupTable[bg].free_blocks_count--;
                // put the updated superblock on disk
                buffer_cache->write_all(1024, sizeof(sb), (char*) &sb);
                uint32_t table_start = (sb.first_data_block + 1) * blockSize;
                buffer_cache->write_all(table_start + bg * sizeof(BlockGroup),
                sizeof(BlockGroup), (char*) &groupTable[bg]);
                return sb.first_data_block + bg * bpg + i;
            }
        }
    }
    // we did not find a suitable block
    return -1;
}
uint32_t Ext2::alloc_inode() {
    char bitmap[blockSize];
    for (uint32_t bg = 0; bg < nGroups; bg++) {
        uint32_t inbmp = groupTable[bg].inode_bitmap;
        buffer_cache->read_all(inbmp * blockSize, blockSize, bitmap);
        uint32_t ipg = sb.inodes_per_group;
        for (uint32_t i = 0; i < ipg; i++) {
            uint32_t byte = i / 8;
            uint32_t bit = i % 8;
            if ((bitmap[byte] & (1 << bit)) == 0) {
                bitmap[byte] |= (1 << bit);
                // put the updated bitmap on disk
                buffer_cache->write_all(inbmp * blockSize, blockSize, bitmap);
                sb.free_inodes_count--;
                groupTable[bg].free_inodes_count--;
                // put the updated superblock on disk
                buffer_cache->write_all(1024, sizeof(sb), (char*) &sb);
                uint32_t table_start = (sb.first_data_block + 1) * blockSize;
                buffer_cache->write_all(table_start + bg * sizeof(BlockGroup),
                sizeof(BlockGroup), (char*) &groupTable[bg]);
                return bg * ipg + i + 1;
            }
        }
    }
    // couldn't find an inode
    return -1;
}


StrongPtr<Node> Ext2::get_node(uint32_t number) {
    if (number == 0) return {};
    ASSERT(number <= numberOfNodes);
    auto index = number - 1;

    auto groupIndex = index / iNodesPerGroup;
    //Debug::printf("groupIndex %d\n",groupIndex);
    ASSERT(groupIndex < nGroups);
    auto indexInGroup = index % iNodesPerGroup;
    auto iTableBase = iNodeTables[groupIndex];
    ASSERT(iTableBase <= numberOfBlocks);
    //Debug::printf("iTableBase %d\n",iTableBase);
    auto nodeOffset = iTableBase * blockSize + indexInGroup * iNodeSize;
    //Debug::printf("nodeOffset %d\n",nodeOffset);

    auto out = StrongPtr<Node>::make(this, buffer_cache,number,blockSize);
    buffer_cache->read(nodeOffset,out->data);
    return out;
}

// used in open create mode and similar things
StrongPtr<Node> Ext2::create_inode(StrongPtr<Node> parent, char* name, uint16_t mode) {
    uint32_t inode = alloc_inode();
    StrongPtr<Node> ret = StrongPtr<Node>::make(this, buffer_cache, inode,
    blockSize);
    bzero(&ret->data,sizeof(NodeData));
    ret->data.mode = mode;
    if ((mode >> 12) == 4) {
        // . and ..
        ret->data.n_links = 2;
    }
    else {
        ret->data.n_links = 1;
    }
    if ((mode >> 12 ) == 4) {
        ret->data.size_low = blockSize;
    }
    else {
        ret->data.size_low = 0;
    }
    buffer_cache->write_all(ret->get_specific_offset(inode), sizeof(NodeData),
    (char*) &ret->data);
    if ((mode >> 12) == 4) {
        parent->data.n_links++;
        buffer_cache->write_all(parent->get_offset(), sizeof(NodeData), (char*) &parent->data);
    }
    parent->update_dir(inode, name, (mode >> 12));
    return ret;
}

StrongPtr<Node> Ext2::find_one(StrongPtr<Node> dir, char* name) {
    uint32_t full_offset = 0;
    while(full_offset < (uint32_t)dir->size_in_bytes()) {
        uint32_t inode_id;
        dir->read<uint32_t>(full_offset + 0, inode_id);

        uint16_t offset;
        dir->read<uint16_t>(full_offset + 4, offset);

        uint8_t name_length;
        dir->read<uint8_t>(full_offset + 6, name_length);
        
        uint8_t i = 0;
        while(true) {
            char c = 0;
            if(i != name_length) {
                dir->read<char>(full_offset + 8 + i, c);
            }
            // Debug::printf("%c\n", c);
            if (c != name[i]) {
                // Debug::printf(" - NOT THE SAME: %d != %d\n", c, name[i]);
                break; // Not the same name
            }
            // found it
            if (i++ >= name_length) {
                return get_node(inode_id);
            }
        }
        full_offset += offset;
    }
    return StrongPtr<Node>{};
}

StrongPtr<Node> Ext2::find(StrongPtr<Node> dir, const char* name) {
    // Debug::printf("BEGINNING OF FIND %s\n", name);
    
    Path path{name};
    StrongPtr<PathString> curr = path.remove();
    uint32_t count = 0;
    
    StrongPtr<Node> node = dir;
    
    while(!(curr == nullptr)) {
        if(count >= 100) return StrongPtr<Node>{}; // break cycles
        
        char* str = curr->get();
        //Debug::printf("HERE 1 %s | %s\n", name, str);
        if(str[0] == '/') {
            //Debug::printf("HERE 2.1 %s | %s\n", name, str);
            node = root;
        } else {
            // Debug::printf("HERE 2.2 %s | %s\n", name, str);
            if(!node->is_dir()) return StrongPtr<Node>{};
            StrongPtr<Node> next_node = find_one(node, str);
            // Debug::printf("HERE 2.22 %s | %s\n", name, str);
            if(next_node == nullptr) return StrongPtr<Node>();
            
            if(next_node->is_symlink()) {
                count++;
            }
            

            // Symlink
            if(!path.isEmpty() && next_node->is_symlink()) {
                char new_path_str[next_node->size_in_bytes() + 1];
                next_node->get_symbol((char*)&new_path_str);
                Path new_path{new_path_str};
                path.merge_symbolic_link(new_path);
                new_path.clear();
            } 
            // File / Dir
            else {
                // Debug::printf("HERE 3 %s | %s\n", name, str);
                node = next_node;
            }

        }

            // Debug::printf("HERE QM %s | %s\n", name, str);

        delete[] str;
        curr = path.remove();
    }

    // Debug::printf("END OF FIND %s\n", name);

    return node;
}


////////////// NodeData //////////////

void NodeData::show(const char* what) {
    Debug::printf("%s\n",what);
    Debug::printf("    mode 0x%x\n",mode);
    Debug::printf("    uid %d\n",uid);
    Debug::printf("    gif %d\n",gid);
    Debug::printf("    n_links %d\n",n_links);
    Debug::printf("    n_sectors %d\n",n_sectors);
}

///////////// Node /////////////

void Node::get_symbol(char* buffer) {
    ASSERT(is_symlink());
    auto sz = size_in_bytes();
    if (sz < 60) {
        memcpy(buffer,&data.direct0,sz);
    } else {
        auto cnt = read_all(0,sz,buffer);
        ASSERT(cnt == sz);
    }
}

void Node::read_block(uint32_t lbn, char* buffer) {
    auto refs_per_block = block_size / 4;

    // follow one level of indirection
    auto follow([this, refs_per_block](uint32_t pbn, uint32_t index) -> uint32_t {
        ASSERT(index < refs_per_block);
        if (pbn == 0) return 0;

        uint32_t out;
        buffer_cache->read(pbn * block_size + index * 4, out);
        return out;
    });

    uint32_t pbn = 0;

    if (lbn < 12) {
        uint32_t* direct = &data.direct0;
        pbn = direct[lbn];
    } else {
        lbn -= 12;
        if (lbn < refs_per_block) {
            pbn = follow(data.indirect_1, lbn); 
        } else {
            lbn -= refs_per_block;
            if (lbn < (refs_per_block * refs_per_block)) {
                auto d1 = lbn / refs_per_block;
                auto d0 = lbn % refs_per_block;
                pbn = follow(follow(data.indirect_2, d1), d0);
            } else {
                lbn -= refs_per_block * refs_per_block;
                auto d0 = lbn % refs_per_block;
                auto t = lbn / refs_per_block;
                auto d1 = t % refs_per_block;
                auto d2 = t / refs_per_block;
                ASSERT(d2 < refs_per_block);
                ASSERT((((d2 * refs_per_block + d1) * refs_per_block) + d0) == lbn);
                pbn = follow(follow(follow(data.indirect_3, d2), d1), d0);
            }
        }
    }

    if (pbn == 0) {
        // zero-filled, sparse
        bzero(buffer, block_size);
    } else {
        auto cnt = buffer_cache->read_all(pbn * block_size, block_size,buffer);
        ASSERT(cnt == block_size);
    }
}

void Node::write_indirect1(uint32_t lbn, const char* buffer) {
    if (data.indirect_1 == 0) {
        data.indirect_1 = fs->alloc_block(); 
    }
   
    char buf[block_size];
    buffer_cache->read_all(data.indirect_1 * block_size, block_size, buf);
    uint32_t * table = reinterpret_cast<uint32_t*> (buf);
    if (table[lbn] == 0) {
        table[lbn] = fs->alloc_block(); 
        buffer_cache->write_all(data.indirect_1 * block_size, block_size, (char*)table);
    }
    uint32_t pbn = table[lbn];
    buffer_cache->write_all(pbn * block_size, block_size, const_cast<char*> (buffer));

}

void Node::write_indirect2(uint32_t d1, uint32_t d0, const char* buffer) {
    if (data.indirect_2 == 0) {
        data.indirect_2 = fs->alloc_block(); 
    }
   
    char buf1[block_size];
    buffer_cache->read_all(data.indirect_2 * block_size, block_size, buf1);
    uint32_t * table = reinterpret_cast<uint32_t*> (buf1);
    if (table[d1] == 0) {
        table[d1] = fs->alloc_block(); 
        buffer_cache->write_all(data.indirect_2 * block_size, block_size, buf1);
    }

    char buf0[block_size];
    buffer_cache->read_all(table[d1] * block_size, block_size, buf0);
    uint32_t* table1 = reinterpret_cast<uint32_t*> (buf0);
    if (table1[d0] == 0){
        table1[d0] = fs->alloc_block(); 
        buffer_cache->write_all(table[d1] * block_size, block_size, buf0);
    }

    uint32_t pbn = table1[d0];
    buffer_cache->write_all(pbn * block_size, block_size, const_cast<char*> (buffer));

}

void Node::write_indirect3(uint32_t d2, uint32_t d1, uint32_t d0, const char* buffer) {
    if (data.indirect_3 == 0) {
        data.indirect_3 = fs->alloc_block(); 
    }
    char buf2[block_size];
    buffer_cache->read_all(data.indirect_3 * block_size, block_size, buf2);
    uint32_t * table2 = reinterpret_cast<uint32_t*> (buf2);
    if (table2[d2] == 0) {
        table2[d2] = fs->alloc_block(); 
        buffer_cache->write_all(data.indirect_3 * block_size, block_size, buf2);
    }

    char buf1[block_size];
    buffer_cache->read_all(table2[d2] * block_size, block_size, buf1);
    uint32_t * table1 = reinterpret_cast<uint32_t*> (buf1);
    if (table1[d1] == 0) {
        table1[d1] = fs->alloc_block();
        buffer_cache->write_all(table2[d2] * block_size, block_size, buf1);
    }

    char buf0[block_size];
    buffer_cache->read_all(table1[d1] * block_size, block_size, buf0);
    uint32_t * table0 = reinterpret_cast<uint32_t*> (buf0);
    if (table0[d0] == 0) {
        table0[d0] = fs->alloc_block();
        buffer_cache->write_all(table1[d1] * block_size, block_size, buf0);
    }
    uint32_t pbn = table0[d0];
    buffer_cache->write_all(pbn * block_size, block_size, const_cast<char*>(buffer));



}

void Node::write_block(uint32_t lbn, const char* buffer) {
    auto refs_per_block = block_size / 4;
    uint32_t newSize = 0;
    // Debug::printf("*** entered write block, lbn is: %d\n", lbn);
    if (lbn < 12) {
        uint32_t* direct = &data.direct0;
        uint32_t& pbn = direct[lbn];
        if (pbn == 0) {
            pbn = fs->alloc_block(); 
        }
        buffer_cache->write_all(pbn * block_size, block_size, const_cast<char*> (buffer));
        uint32_t newSize = (lbn + 1) * block_size;
        if (newSize > data.size_low) {
            data.size_low = newSize;
        }
        buffer_cache->write_all(get_offset(), sizeof(NodeData), (char*) &data);
    }
    else {
        lbn -= 12;
        if (lbn < refs_per_block) {
            write_indirect1(lbn, buffer);
            newSize = (lbn + 12 + 1) * block_size;
        }
        else {
            lbn -= refs_per_block;
            if (lbn < (refs_per_block * refs_per_block)) {
                uint32_t d1 = lbn / refs_per_block;
                uint32_t d0 = lbn % refs_per_block;
                write_indirect2(d1, d0, buffer);
                newSize = (lbn + 12 + 1 + refs_per_block) * block_size;
            }
            else {
                lbn -= refs_per_block * refs_per_block;
                auto d0 = lbn % refs_per_block;
                auto t = lbn / refs_per_block;
                auto d1 = t % refs_per_block;
                auto d2 = t / refs_per_block;
                write_indirect3(d2, d1, d0, buffer);
                newSize = (lbn + 12 + 1 + refs_per_block + (refs_per_block * refs_per_block)) * block_size;

            }
        }
        if (newSize > data.size_low) {
            data.size_low = newSize;
        }
        buffer_cache->write_all(get_offset(), sizeof(NodeData), (char*) &data);
    }
}

void Node::update_dir(uint32_t entry_number, char* name, uint16_t type) {
    uint32_t dir_block = 0;
    if (data.size_low > 0) {
        dir_block =  (data.size_low - 1) / block_size;
    }
    uint32_t offset = dir_block * block_size;
    char* block = new char[block_size];
    read_block(dir_block, block);

    // Get the last entry
    // The last entry has a rec lan that spans the rest of the block
    // We need to adjust this
    uint32_t curr = 0;
    uint32_t prev = 0;
    struct DirectoryEntry
    {
        uint32_t inode;
        uint16_t rec_len;
        uint8_t name_len;
        uint8_t file_type;
    };
    DirectoryEntry* t = nullptr;
    while (curr < block_size) {
        t = (DirectoryEntry*) (block + curr);
        prev = curr;
        curr += t->rec_len;
    }
    uint16_t add = 0;
    uint16_t name_len = t->name_len;
    if (name_len % 4 > 0) {
        add += 4 - (name_len % 4);
    }
    uint16_t adjusted = 8 + name_len + add;
    t->rec_len = adjusted;
    write_block(dir_block, block);

    // place the new entry here
    uint32_t place = offset + prev + adjusted;
    name_len = K::strlen(name);
    uint32_t rec_len = block_size - ((place - offset) % block_size);
   
    DirectoryEntry de;
    de.inode = entry_number;
    de.rec_len = rec_len;
    de.name_len = name_len;
    // switch statement on type
    switch (type) {
        case 4:
            de.file_type = 2;
            break;
        case 8:
            de.file_type = 1;
            break;
        case 0xA:
            de.file_type = 7;
            break;
        case 0x2:
            de.file_type = 3;
            break;
        case 0x6:
            de.file_type = 4;
            break;
        case 0xC:
            de.file_type = 6;
            break;
        case 1:
            de.file_type = 5;
            break;
        default:
            de.file_type = 0;
            break;
    }
    this->write_all(place, sizeof(de), (char*) &de);
    this->write_all(place + 8, name_len, name);
    data.size_low = place + rec_len;
    buffer_cache->write_all(get_offset(), sizeof(NodeData), (char*) &data);
}
uint32_t Node::entry_count() {
    ASSERT(is_dir());
    uint32_t count = 0;
    entries([&count](uint32_t,const char*,uint32_t) -> uint32_t {
        count += 1;
        return 0;
    });
    return count;
}

// new code
void Node::truncate(uint32_t new_length) {
    if (!is_file()) {
        return;  
    }

    if (new_length >= data.size_low) {
        // just update the size
        data.size_low = new_length;
        buffer_cache->write_all(get_offset(), sizeof(NodeData), (char*)&data);
        return;
    }

    // how many blocks we need to keep and deallocate the rest
    uint32_t current_block_count = (data.size_low + block_size - 1) / block_size;
    uint32_t new_block_count = (new_length + block_size - 1) / block_size;
    
    // free direct blocks 
    uint32_t* direct = &data.direct0;
    for (uint32_t i = new_block_count; i < 12 && i < current_block_count; i++) {
        if (direct[i] != 0) {
            uint32_t block_num = direct[i];
            uint32_t group = block_num / fs->sb.blocks_per_group;
            uint32_t block_index = block_num % fs->sb.blocks_per_group;
            // mark block as free in block bitmap
            char bitmap[fs->blockSize];
            uint32_t block_bitmap = fs->groupTable[group].block_bitmap;
            buffer_cache->read_all(block_bitmap * fs->blockSize, fs->blockSize, bitmap);
            
            uint32_t byte = block_index / 8;
            uint32_t bit = block_index % 8;
            
            bitmap[byte] &= ~(1 << bit);  // clear the bit
            
            buffer_cache->write_all(block_bitmap * fs->blockSize, fs->blockSize, bitmap);
            
            // update sb and gdt
            fs->sb.free_blocks_count++;
            fs->groupTable[group].free_blocks_count++;
            
            // clear the direct pointer
            direct[i] = 0;
        }
    }
    
    // free first indirect blocks
    if (new_block_count < 12 && current_block_count > 12 && data.indirect_1 != 0) {
        // deallocate all indirect blocks
        uint32_t refs_per_block = block_size / 4;
        uint32_t* indirect_block = new uint32_t[refs_per_block];
        buffer_cache->read_all(data.indirect_1 * block_size, block_size, (char*)indirect_block);
        
        // free all blocks it points to
        for (uint32_t i = 0; i < refs_per_block; i++) {
            if (indirect_block[i] != 0) {
                uint32_t block_num = indirect_block[i];
                uint32_t group = block_num / fs->sb.blocks_per_group;
                uint32_t block_index = block_num % fs->sb.blocks_per_group;
                // mark block as free in block bitmap 
                char bitmap[fs->blockSize];
                uint32_t block_bitmap = fs->groupTable[group].block_bitmap;
                buffer_cache->read_all(block_bitmap * fs->blockSize, fs->blockSize, bitmap);
                
                uint32_t byte = block_index / 8;
                uint32_t bit = block_index % 8;
                
                bitmap[byte] &= ~(1 << bit);  // clear the bit
                
                buffer_cache->write_all(block_bitmap * fs->blockSize, fs->blockSize, bitmap);
                
                // update sb and gdt
                fs->sb.free_blocks_count++;
                fs->groupTable[group].free_blocks_count++;
            }
        }
        
        delete[] indirect_block; // dont need this anymore !
        
        // free the indirect block itself, same process again
        uint32_t group = data.indirect_1 / fs->sb.blocks_per_group;
        uint32_t block_index = data.indirect_1 % fs->sb.blocks_per_group;
        // mark block as free in block bitmap 
        char bitmap[fs->blockSize];
        uint32_t block_bitmap = fs->groupTable[group].block_bitmap;
        buffer_cache->read_all(block_bitmap * fs->blockSize, fs->blockSize, bitmap);
        
        uint32_t byte = block_index / 8;
        uint32_t bit = block_index % 8;
        
        bitmap[byte] &= ~(1 << bit);  // clear the bit
        
        buffer_cache->write_all(block_bitmap * fs->blockSize, fs->blockSize, bitmap);
        
        // update sb and gdt
        fs->sb.free_blocks_count++;
        fs->groupTable[group].free_blocks_count++;
        
        data.indirect_1 = 0;
    } else if (new_block_count >= 12 && data.indirect_1 != 0) {
        // need to keep some indirect blocks but free others
        uint32_t refs_per_block = block_size / 4;
        uint32_t* indirect_block = new uint32_t[refs_per_block];
        buffer_cache->read_all(data.indirect_1 * block_size, block_size, (char*)indirect_block);
        
        // free blocks beyond the new_block_count
        for (uint32_t i = new_block_count - 12; i < refs_per_block; i++) {
            if (indirect_block[i] != 0) {
                uint32_t block_num = indirect_block[i];
                uint32_t group = block_num / fs->sb.blocks_per_group;
                uint32_t block_index = block_num % fs->sb.blocks_per_group;
                // mark block as free in block bitmap 
                char bitmap[fs->blockSize];
                uint32_t block_bitmap = fs->groupTable[group].block_bitmap;
                buffer_cache->read_all(block_bitmap * fs->blockSize, fs->blockSize, bitmap);
                
                uint32_t byte = block_index / 8;
                uint32_t bit = block_index % 8;
                
                bitmap[byte] &= ~(1 << bit);  // clear the bit
                
                buffer_cache->write_all(block_bitmap * fs->blockSize, fs->blockSize, bitmap);
                
                // update sb and gdt
                fs->sb.free_blocks_count++;
                fs->groupTable[group].free_blocks_count++;
                
                // clear the pointer in the indirect block
                indirect_block[i] = 0;
            }
        }
        
        // write the new indirect block
        buffer_cache->write_all(data.indirect_1 * block_size, block_size, (char*)indirect_block);
        
        delete[] indirect_block;
    }
    
    // TODO: still need to handle double and triple indirect blocks 
    // follow the same pattern as the first indirect
    
    // update everything
    data.size_low = new_length;
    data.n_sectors = (new_length + 511) / 512; 
    buffer_cache->write_all(get_offset(), sizeof(NodeData), (char*)&data); // write the updated inode back to disk
    buffer_cache->write_all(1024, sizeof(fs->sb), (char*)&fs->sb); // update sb
    // update gdt
    uint32_t table_start = (fs->sb.first_data_block + 1) * fs->blockSize;
    for (uint32_t i = 0; i < fs->nGroups; i++) {
        buffer_cache->write_all(table_start + i * sizeof(BlockGroup),
                              sizeof(BlockGroup), (char*)&fs->groupTable[i]);
    }
}

