require "nvchad.options"

-- add yours here!

-- local o = vim.o
-- o.cursorlineopt ='both' -- to enable cursorline!

vim.cmd("let g:netrw_liststyle = 3")
vim.cmd "syntax enable"
vim.cmd "filetype plugin indent on"

local ts_runtime = vim.fn.stdpath "data" .. "/lazy/nvim-treesitter/runtime"
if vim.uv.fs_stat(ts_runtime) then
  vim.opt.runtimepath:append(ts_runtime)
end

local opt = vim.opt

opt.relativenumber = true
opt.number = true

opt.mouse = ""
-- cursor

-- tabs & indentation
opt.tabstop = 4 -- 2 spaces for tabs (prettier default)
opt.shiftwidth = 4 -- 2 spaces for indent width
opt.expandtab = true -- expand tab to spaces
opt.autoindent = false -- disable auto indentation
opt.cindent = false -- disable C-style indentation
opt.smartindent = false -- disable smart indentation

opt.wrap = true

-- search settings
opt.ignorecase = true -- ignore case when searching
opt.smartcase = true -- if you include mixed case in your search, assumes you want case-sensitive

opt.cursorline = true

-- turn on termguicolors for tokyonight colorscheme to work
-- (have to use iterm2 or any other true color terminal)
opt.termguicolors = true
opt.background = "dark" -- colorschemes that can be light or dark will be made dark
opt.signcolumn = "yes" -- show sign column so that text doesn't shift

-- backspace
opt.backspace = "indent,eol,start" -- allow backspace on indent, end of line or insert mode start position

-- clipboard
-- opt.clipboard:append("unnamedplus") -- use system clipboard as default register

-- split windows
opt.splitright = true -- split vertical window to the right
opt.splitbelow = true -- split horizontal window to the bottom

-- turn off swapfile
opt.swapfile = false

opt.scrolloff = 5

-- Enable Treesitter folding
vim.opt.foldmethod = "expr"
vim.opt.foldexpr = "nvim_treesitter#foldexpr()"

-- Don't auto-fold by default
vim.opt.foldenable = false
vim.opt.foldlevel = 99

-- Optional: Configure fold display
vim.opt.foldnestmax = 10 -- Maximum fold nesting level
vim.opt.foldminlines = 1 -- Minimum lines for a fold to be displayed
