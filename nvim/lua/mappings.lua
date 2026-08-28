require "nvchad.mappings"

-- add yours here
local map = vim.keymap.set

map("n", ";", ":", { desc = "CMD enter command mode" })
map("i", "jk", "<ESC>")

-- Visually select all lines
map("n", "<C-a>", "ggVG", { desc = "Visually select all lines" })

-- Smart tab function to prioritize completion, then snippets, then exiting brackets/quotes
local function smart_tab()
  local has_cmp, cmp = pcall(require, "cmp")
  if has_cmp and cmp.visible() then
    cmp.select_next_item()
    return
  end

  local has_luasnip, luasnip = pcall(require, "luasnip")
  if has_luasnip and luasnip.expand_or_jumpable() then
    luasnip.expand_or_jump()
    return
  end

  local line = vim.api.nvim_get_current_line()
  local col = vim.api.nvim_win_get_cursor(0)[2]
  local char_at_cursor = line:sub(col + 1, col + 1)
  local exit_chars = { '"', "'", ")", "}", ">", "]" }

  for _, char in ipairs(exit_chars) do
    if char_at_cursor == char then
      vim.api.nvim_win_set_cursor(0, {vim.api.nvim_win_get_cursor(0)[1], col + 1})
      return
    end
  end

  vim.api.nvim_feedkeys(vim.api.nvim_replace_termcodes("<Tab>", true, false, true), "n", false)
end

-- Map Tab to smart_tab function in insert mode
map("i", "<Tab>", smart_tab, { desc = "Smart tab - exit brackets/quotes or normal tab" })

-- Optional: Map Shift+Tab for reverse completion navigation if you use completion
map("i", "<S-Tab>", function()
  local has_luasnip, luasnip = pcall(require, "luasnip")
  if has_luasnip and luasnip.jumpable(-1) then
    luasnip.jump(-1)
    return
  end

  local has_cmp, cmp = pcall(require, "cmp")
  if has_cmp and cmp.visible() then
    cmp.select_prev_item()
  else
    vim.api.nvim_feedkeys(vim.api.nvim_replace_termcodes("<S-Tab>", true, false, true), "n", false)
  end
end, { desc = "Reverse tab completion" })

-- Copy to system clipboard in visual mode with Ctrl+c
map("v", "<C-c>", '"+y', { desc = "Copy to system clipboard" })

-- Paste from system clipboard in normal mode with Ctrl+v
map("n", "<C-v>", '"+p', { desc = "Paste from system clipboard" })

-- Paste from system clipboard in insert mode with Ctrl+v
map("i", "<C-v>", '<C-r>+', { desc = "Paste from system clipboard (insert)" })

-- Paste from system clipboard in visual mode with Ctrl+v (replaces selection)
map("v", "<C-v>", '"+p', { desc = "Paste from system clipboard (visual)" })

-- Keybindings for methods/functions using nvim-treesitter-textobjects
map("n", "yim", "yif", { desc = "Yank inside method/function" })
map("n", "dam", "daf", { desc = "Delete around method/function" })
map("n", "cim", "cif", { desc = "Change/Clear inside method/function" })

-- writing and quit files
map("n","<ESC>","<cmd>q<CR>",{desc= "Quit files with ESC"})

-- COMPILE & RUN MAPPINGS:

-- Alternative mapping with leader key (reliable)
map("n", "<leader>rr", function()
  require("configs.compile_run").compile_and_run()
end, { desc = "Compile and run current file" })

