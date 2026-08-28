local success, ts_configs = pcall(require, "nvim-treesitter.configs")
if success then
  print(vim.inspect(ts_configs))
end